#include "libcmt/sock.h"
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

// send up to 64 file descriptors at once
#define CMT_CMSG_MAX (64 * sizeof(int))

int cmt_sock_addr_init(struct sockaddr_un *me, size_t length, const char path[]) {
    if (length > sizeof(me->sun_path)-1U) {
        return -ENOBUFS;
    }
    me->sun_family = AF_UNIX;
    memset(me->sun_path, 0, sizeof(me->sun_path));
    memcpy(me->sun_path, path, length+1); // include '\0'
    return 0;
}
static int send_fds(int fd, struct sockaddr_un *addr, size_t length, int fds[]) {
    char cmsg[CMSG_SPACE(length * sizeof(int))]; // TODO: remove VLA
    struct msghdr msg[1] = {{
        .msg_name    = addr,
        .msg_namelen = sizeof(*addr),
        .msg_iov     = NULL,
        .msg_iovlen  = 0,
        .msg_control = cmsg,
        .msg_controllen = sizeof(cmsg),
    }};

    struct cmsghdr *c = CMSG_FIRSTHDR(msg);
    c->cmsg_level = SOL_SOCKET;
    c->cmsg_type  = SCM_RIGHTS;
    c->cmsg_len   = CMSG_LEN(length * sizeof(int));
    memcpy(CMSG_DATA(c), fds, length * sizeof(int));
    ssize_t rc = sendmsg(fd, msg, 0);
    if (rc == -1) { return -errno; }
    return 0;
}
static int recv_fds(int fd, struct sockaddr_un *addr, size_t length, int fds[]) {
    char cmsg[CMSG_SPACE(length * sizeof(int))]; // TODO: remove VLA
    struct msghdr msg[1] = {{
        .msg_name    = (struct sockaddr *)addr,
        .msg_namelen = sizeof(*addr),
        .msg_iov     = NULL,
        .msg_iovlen  = 0,
        .msg_control = cmsg,
        .msg_controllen = sizeof(cmsg),
    }};
    ssize_t rc = recvmsg(fd, msg, 0);
    if (rc == -1) { return -errno; }

    struct cmsghdr *c = CMSG_FIRSTHDR(msg);
    memcpy(fds, CMSG_DATA(c), length * sizeof(int));
    return 0;
}
const char *cmt_sock_client_get_path(void)
{
    const char *env = getenv("CMT_SOCK");
    return env? env : "/tmp/cmt.client.sock";
}
const char *cmt_sock_server_get_path(void)
{
    const char *env = getenv("CMT_SOCK");
    return env? env : "/tmp/cmt.server.sock";
}

int cmt_sock_init(cmt_conn_t *me, const char *addr, const char *other)
{
    int rc = 0;

    rc = cmt_sock_addr_init(&me->addr.un, strlen(addr), addr);
    if (rc) {
        return rc;
    }
    rc = cmt_sock_addr_init(&me->other.un, strlen(other), other);
    if (rc) {
        return rc;
    }

    me->fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (me->fd < 0) {
        return -errno;
    }

    rc = bind(me->fd, (struct sockaddr *)&me->addr.un, sizeof(me->addr.un));
    if (rc < 0) {
        close(me->fd);
        return -errno;
    }
    return 0;
}

void cmt_sock_fini(cmt_conn_t *me)
{
    unlink(me->addr.un.sun_path);
    close(me->fd);
    me->fd = -1;
}

int cmt_sock_send(cmt_conn_t *me, size_t length, const void *data)
{
    ssize_t rc = sendto(me->fd, data, length, 0, (struct sockaddr *)&me->other.un, sizeof(me->other.un));
    if (rc < 0) { return -errno; }
    return rc;
}

int cmt_sock_recv(cmt_conn_t *me, size_t length, void *data)
{
    ssize_t rc = recv(me->fd, data, length, 0);
    if (rc < 0) { return -errno; }
    return rc;
}

int cmt_sock_send_fds(cmt_conn_t *me, size_t length, int fds[])
{
    return send_fds(me->fd, &me->other.un, length, fds);
}

int cmt_sock_recv_fds(cmt_conn_t *me, size_t length, int fds[])
{
    return recv_fds(me->fd, &me->addr.un, length, fds);
}
