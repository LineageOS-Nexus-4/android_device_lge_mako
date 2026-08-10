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

static int send_msg(struct nlmsghdr *req, int len, const char *what)
{
	int fd, ret = 0;
	struct sockaddr_nl sa;
	char buf[2048];

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) { perror("socket"); return 1; }
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) { perror("bind"); return 1; }
	if (sendto(fd, req, len, 0, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("sendto"); close(fd); return 1;
	}
	{
		int rlen = recv(fd, buf, sizeof(buf), 0);
		if (rlen < 0) { perror("recv"); close(fd); return 1; }
		struct nlmsghdr *nh = (struct nlmsghdr *)buf;
		if (nh->nlmsg_type == NLMSG_ERROR) {
			struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nh);
			if (err->error == 0)
				printf("RESULT: %s -> ACK ok\n", what);
			else {
				printf("RESULT: %s -> error %d (%s)\n", what,
				       err->error, strerror(-err->error));
				ret = 1;
			}
		} else {
			printf("RESULT: %s -> unexpected reply %d\n", what, nh->nlmsg_type);
			ret = 1;
		}
	}
	close(fd);
	return ret;
}

/* netd-style RTM_NEWROUTE: rtmsg with table + RTA_TABLE, RTA_OIF, RTA_PRIORITY, RTA_METRICS */
static void route_add(int ifindex, const char *dst, int plen, const char *gw, u_int32_t table,
		      int with_rtatable, int with_metrics)
{
	struct {
		struct nlmsghdr n;
		struct rtmsg r;
		char buf[256];
	} req;
	u_int32_t ip = 0, oif = ifindex, prio = 0x00000000, mtu = 1500;
	struct rtattr *metrics;

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	req.n.nlmsg_type = RTM_NEWROUTE;
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_REPLACE;
	req.n.nlmsg_seq = 1;
	req.r.rtm_family = AF_INET;
	req.r.rtm_dst_len = plen;
	req.r.rtm_table = table;
	req.r.rtm_protocol = RTPROT_STATIC;
	req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	req.r.rtm_type = RTN_UNICAST;

	if (dst) {
		inet_pton(AF_INET, dst, &ip);
		add_attr(&req.n, sizeof(req), RTA_DST, &ip, 4);
	}
	if (gw) {
		inet_pton(AF_INET, gw, &ip);
		add_attr(&req.n, sizeof(req), RTA_GATEWAY, &ip, 4);
	}
	add_attr(&req.n, sizeof(req), RTA_OIF, &oif, 4);
	add_attr(&req.n, sizeof(req), RTA_PRIORITY, &prio, 4);
	if (with_metrics) {
		metrics = (struct rtattr *)(((char *)&req.n) + NLMSG_ALIGN(req.n.nlmsg_len));
		metrics->rta_type = RTA_METRICS;
		metrics->rta_len = RTA_LENGTH(0);
		req.n.nlmsg_len = NLMSG_ALIGN(req.n.nlmsg_len) + RTA_LENGTH(0);
		add_attr(&req.n, sizeof(req), RTAX_MTU, &mtu, 4);
		metrics->rta_len = (char *)&req.n + req.n.nlmsg_len - (char *)metrics;
	}
	if (with_rtatable)
		add_attr(&req.n, sizeof(req), RTA_TABLE, &table, 4);

	send_msg(&req.n, req.n.nlmsg_len, "route add");
}

int main(void)
{
	int ifindex = if_nametoindex("wlan0");
	if (!ifindex) { perror("if_nametoindex"); return 1; }
	printf("wlan0 ifindex %d\n", ifindex);

	route_add(ifindex, "192.168.0.0", 24, NULL, 104, 1, 1); /* netd style w/ RTA_TABLE+METRICS */
	route_add(ifindex, "192.168.0.0", 24, NULL, 104, 0, 1); /* no RTA_TABLE */
	route_add(ifindex, "192.168.0.0", 24, NULL, 104, 1, 0); /* no METRICS */
	route_add(ifindex, "192.168.0.0", 24, NULL, 104, 0, 0); /* minimal */
	route_add(ifindex, "0.0.0.0", 0, "192.168.0.1", 104, 1, 1); /* default via gw */
	return 0;
}
