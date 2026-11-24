// SPDX-License-Identifier: Apache-2.0
// Restore mako's foreground CPU groups after legacy cpuset hotplug.
#include <errno.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/system_properties.h>
#include <unistd.h>

static const char *const paths[] = {
    "/dev/cpuset/foreground/cpus",
    "/dev/cpuset/foreground_window/cpus",
    "/dev/cpuset/top-app/cpus",
    "/dev/cpuset/system-background/cpus",
};

#define GROUP_COUNT (sizeof(paths) / sizeof(paths[0]))
#define MASK_LEN 32

static bool read_mask(const char *path, char mask[MASK_LEN]) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return false;
    ssize_t length = read(fd, mask, MASK_LEN - 1);
    close(fd);
    if (length < 1 || length >= MASK_LEN - 1) return false;
    mask[length] = '\0';
    mask[strcspn(mask, "\r\n")] = '\0';
    return mask[0] && strspn(mask, "0123456789,-") == strlen(mask);
}

static bool write_mask(const char *path, const char *mask) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) return false;
    char line[MASK_LEN];
    int length = snprintf(line, sizeof(line), "%s\n", mask);
    bool ok = length > 0 && length < (int)sizeof(line) && write(fd, line, length) == length;
    close(fd);
    return ok;
}

static void refresh_groups(void) {
    char active[MASK_LEN], current[MASK_LEN];
    if (!read_mask("/dev/cpuset/cpus", active)) return;
    for (size_t i = 0; i < GROUP_COUNT; ++i) {
        if (read_mask(paths[i], current) && strcmp(active, current)) {
            // The root cpuset may lag the CPU notifier. The next retry uses a fresh mask.
            write_mask(paths[i], active);
        }
    }
}

static bool is_cpu_hotplug(const char *event) {
    const char *prefixes[] = {
        "online@/devices/system/cpu/cpu", "offline@/devices/system/cpu/cpu",
    };
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        size_t length = strlen(prefixes[i]);
        if (!strncmp(event, prefixes[i], length)
                && event[length] >= '0' && event[length] <= '3'
                && event[length + 1] == '\0') return true;
    }
    return false;
}

int main(void) {
    char device[PROP_VALUE_MAX];
    if (getuid() != 0 || !__system_property_get("ro.product.device", device)
            || strcmp(device, "mako")) {
        fprintf(stderr, "The cpuset service requires root on mako.\n");
        return 1;
    }

    sigset_t stop_signals;
    sigemptyset(&stop_signals);
    sigaddset(&stop_signals, SIGINT);
    sigaddset(&stop_signals, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &stop_signals, NULL) < 0) {
        perror("sigprocmask");
        return 1;
    }
    int stop_fd = signalfd(-1, &stop_signals, SFD_CLOEXEC);
    int socket_fd = socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT);
    struct sockaddr_nl kernel_events = { .nl_family = AF_NETLINK, .nl_groups = 1 };
    if (stop_fd < 0 || socket_fd < 0 ||
            bind(socket_fd, (const struct sockaddr *)&kernel_events, sizeof(kernel_events)) < 0) {
        perror("Open hotplug monitor");
        return 1;
    }
    struct pollfd fds[] = {
        { .fd = socket_fd, .events = POLLIN },
        { .fd = stop_fd, .events = POLLIN },
    };
    fprintf(stderr, "Following CPU hotplug without changing power or thermal limits.\n");
    int rc = 0;
    int retries = 4;
    refresh_groups();
    for (;;) {
        int ready = poll(fds, 2, retries ? 60 : -1);
        if (ready < 0 && errno == EINTR) continue;
        if (ready < 0 || (fds[0].revents & (POLLHUP | POLLNVAL))) {
            rc = 1;
            break;
        }
        if (fds[1].revents) break;
        if (ready == 0) {
            refresh_groups();
            --retries;
        } else if (fds[0].revents & (POLLIN | POLLERR)) {
            struct sockaddr_nl sender = {0};
            socklen_t sender_len = sizeof(sender);
            char event[2048];
            ssize_t n = recvfrom(socket_fd, event, sizeof(event) - 1, MSG_DONTWAIT,
                                 (struct sockaddr *)&sender, &sender_len);
            if (n > 0 && sender.nl_pid == 0) {
                event[n] = '\0';
                if (!is_cpu_hotplug(event)) continue;
            } else if (n < 0 && errno == ENOBUFS) {
                // A lost notification is also a cue to reconcile with the kernel.
            } else {
                continue;
            }
            refresh_groups();
            retries = 4;
        }
    }

    close(socket_fd);
    close(stop_fd);
    return rc;
}
