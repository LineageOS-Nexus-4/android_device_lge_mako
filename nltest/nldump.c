#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

int main(void)
{
	int fd, i;
	struct sockaddr_nl sa;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) { perror("socket"); return 1; }

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE |
			RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE;
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) { perror("bind"); return 1; }
	printf("listening...\n");

	for (;;) {
		char buf[8192];
		ssize_t len = recv(fd, buf, sizeof(buf), 0);
		if (len < 0) { perror("recv"); break; }
		printf("=== recv %zd bytes\n", len);
		for (i = 0; i < len; i++) {
			printf("%02x", (unsigned char)buf[i]);
			if ((i + 1) % 16 == 0) printf("\n");
			else if ((i + 1) % 4 == 0) printf(" ");
		}
		if (len % 16) printf("\n");
		fflush(stdout);
	}
	return 0;
}
