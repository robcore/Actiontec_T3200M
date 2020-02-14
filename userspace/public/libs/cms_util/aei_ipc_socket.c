#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "aei_ipc_socket.h"

int ipc_socket(int type, unsigned int src_ip, unsigned short src_port)
{
    int sock = -1;
    struct sockaddr_in sa;

    if ((sock = socket(AF_INET, type, 0))<0)
    {
        fprintf(stderr, "%s: failed socket()\n", __func__);
        return -1;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = src_ip;
    sa.sin_port = htons(src_port); 
    if (src_port)
    {
        int opt = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
                    (void *)&opt, sizeof(opt))<0)
        {
            fprintf(stderr, "%s: failed setsockopt(SO_REUSEADDR)\n", __func__);
            goto Error;
        }
        if ((bind(sock, (struct sockaddr *)&sa, sizeof(sa)))<0)
            goto BindError;
    }
    else
    {
        if ((bind(sock, (struct sockaddr *)&sa, sizeof(sa)))<0)
            goto BindError;
    }

    return sock;

BindError:
    fprintf(stderr, "%s: failed bind() for ip %s port %d: %s\n", __func__,
            inet_ntoa(sa.sin_addr), ntohs(sa.sin_port), strerror(errno));
Error:
    close(sock);
    return -1;
}

int ipc_listen(unsigned short port, unsigned int addr)
{
    int fd;

    if ((fd = ipc_socket(SOCK_STREAM, addr, port)) < 0)
        return -1;

    if (listen(fd, 5) < 0)
    {
        fprintf(stderr, "%s: Failed ipc listen %m\n", __func__);
        goto Error;
    }
    return fd;

Error:
    close(fd);
    return -1;
}

int ipc_accept(int server_fd)
{
    int fd;

    while ((fd = accept(server_fd, NULL, NULL)) < 0)
    {
        if (errno != EINTR && errno != EAGAIN) 
            return -1;
    }
    return fd;
}

int ipc_connect(unsigned short port, unsigned int addr)
{
    struct sockaddr_in sa;
    int fd = -1;

    if ((fd = ipc_socket(SOCK_STREAM, 0, 0)) < 0)
        return -1;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = addr;
    sa.sin_port = htons(port); 

    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}

int ipc_write(int fd, void *buf, size_t count)
{
    int rc, total = 0;

    while (total < count)
    {
        rc = write(fd, buf + total, count - total);
        if (rc < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        else if (!rc)
            return -1;

        total += rc;
    }

    return 0;
}

int ipc_read(int fd, void *buf, size_t count)
{
    int rc, total = 0;

    while (total!=count)
    {
        rc = read(fd, buf + total, count - total);
        if (rc < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        else if (!rc)
            return -1;

        total += rc;
    }

    return 0;
}

void aei_send_ipc(unsigned short port, Tr69MonitorMsgBody *msg)
{
    int fd;

    if ((fd = ipc_connect(port, inet_addr("127.0.0.1"))) < 0)
    {
        fprintf(stderr, "Failed to connect!\n");
        return;
    }

    ipc_write(fd, (unsigned char *)msg, sizeof(Tr69MonitorMsgBody));
    close(fd);
}

