#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <arpa/inet.h>

static void add_attr(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;
	rta = (struct rtattr *)(((char *)n) + NLMSG_ALIGN(n->nlmsg_len));
	rta->rta_type = type;
	rta->rta_len = len;
	if (alen)
		memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);
}

static int send_rtm_newaddr(int ifindex, const char *addr, int prefix,
			    int nlflags, int with_flags_attr, int with_cacheinfo)
{
	int fd, ret = 0;
	struct {
		struct nlmsghdr n;
		struct ifaddrmsg ifa;
		char buf[256];
	} req;
	struct sockaddr_nl sa;
	struct ifa_cacheinfo ci = { 7200, 7200, 0, 0 };
	u_int32_t ip, flagsattr = 0;

	inet_pton(AF_INET, addr, &ip);

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.n.nlmsg_type = RTM_NEWADDR;
	req.n.nlmsg_flags = nlflags;
	req.n.nlmsg_seq = 1;
	req.ifa.ifa_family = AF_INET;
	req.ifa.ifa_prefixlen = prefix;
	req.ifa.ifa_scope = RT_SCOPE_UNIVERSE;
	req.ifa.ifa_index = ifindex;

	add_attr(&req.n, sizeof(req), IFA_ADDRESS, &ip, 4);
	if (with_cacheinfo)
		add_attr(&req.n, sizeof(req), IFA_CACHEINFO, &ci, sizeof(ci));
	if (with_flags_attr)
		add_attr(&req.n, sizeof(req), IFA_FLAGS, &flagsattr, 4);
	add_attr(&req.n, sizeof(req), IFA_LOCAL, &ip, 4);
	{
		u_int32_t bcast = ip | htonl(0x00ffffff);
		add_attr(&req.n, sizeof(req), IFA_BROADCAST, &bcast, 4);
	}

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) { perror("socket"); return 1; }

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) { perror("bind"); return 1; }

	if (sendto(fd, &req, req.n.nlmsg_len, 0, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("sendto"); return 1;
	}

	{
		char buf[1024];
		int len = recv(fd, buf, sizeof(buf), 0);
		if (len < 0) { perror("recv"); return 1; }
		struct nlmsghdr *nh = (struct nlmsghdr *)buf;
		if (nh->nlmsg_type == NLMSG_ERROR) {
			struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nh);
			if (err->error == 0) {
				printf("RESULT: ACK ok (flags we sent: 0x%x)\n", nlflags);
			} else {
				printf("RESULT: error %d (%s) (flags we sent: 0x%x)\n",
				       err->error, strerror(-err->error), nlflags);
				ret = 1;
			}
		} else {
			printf("RESULT: unexpected reply type %d\n", nh->nlmsg_type);
			ret = 1;
		}
	}
	close(fd);
	return ret;
}

int main(int argc, char **argv)
{
	int ifindex = if_nametoindex("wlan0");
	if (!ifindex) { perror("if_nametoindex"); return 1; }
	printf("wlan0 ifindex %d\n", ifindex);

	/* 1. exact networkstack message: REQUEST|REPLACE|ACK, IFA_FLAGS + IFA_CACHEINFO */
	send_rtm_newaddr(ifindex, "192.168.0.137", 24,
			 NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_ACK, 1, 1);
	/* 2. same but without IFA_FLAGS */
	send_rtm_newaddr(ifindex, "192.168.0.137", 24,
			 NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_ACK, 0, 1);
	/* 3. same but without IFA_CACHEINFO */
	send_rtm_newaddr(ifindex, "192.168.0.137", 24,
			 NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_ACK, 1, 0);
	/* 4. with CREATE|EXCL like iproute2 */
	send_rtm_newaddr(ifindex, "192.168.0.137", 24,
			 NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL, 1, 1);
	return 0;
}
