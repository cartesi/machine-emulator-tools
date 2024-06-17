#ifndef CMT_SOCK_H
#define CMT_SOCK_H
#include <sys/socket.h>
#include <sys/un.h>

typedef union {
    struct sockaddr_storage ss;
    struct sockaddr_un      un;
} cmt_sock_addr_t;

typedef struct {
    cmt_sock_addr_t addr, other;
    int fd;
} cmt_conn_t;

const char *cmt_sock_client_get_path(void);
const char *cmt_sock_server_get_path(void);

int  cmt_sock_addr_init(struct sockaddr_un *me, size_t length, const char path[]);

int  cmt_sock_init(cmt_conn_t *me, const char *self, const char *other);
void cmt_sock_fini(cmt_conn_t *me);

int  cmt_sock_send(cmt_conn_t *me, size_t length, const void *data);
int  cmt_sock_recv(cmt_conn_t *me, size_t length, void *data);

int  cmt_sock_send_fds(cmt_conn_t *me, size_t length, int fds[]);
int  cmt_sock_recv_fds(cmt_conn_t *me, size_t length, int fds[]);


#endif /* CMT_SOCK_H */
