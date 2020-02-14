#ifndef __AEI_IPC_SOCKET_H__
#define __AEI_IPC_SOCKET_H__

typedef enum
{
    TR69_MONITOR_MSG_SESSION_START              = 0x01,
    TR69_MONITOR_MSG_SESSION_STOP               = 0x02,
    TR69_MONITOR_MSG_PERIODIC_INFORM_SENT       = 0x04,
    TR69_MONITOR_MSG_PERIODIC_INFORM_DISABLED   = 0x08,
    TR69_MONITOR_MSG_PERIODIC_INFORM_ENABLED    = 0x10,
    TR69_MONITOR_MSG_CONNECTION_REQUEST_START   = 0x20,
    TR69_MONITOR_MSG_CONNECTION_REQUEST_SENT    = 0x40
} Tr69MonitorMsgType;

typedef struct {
    unsigned int messageTypes;
} Tr69MonitorMsgBody;

int ipc_socket(int type, unsigned int src_ip, unsigned short src_port);
int ipc_listen(unsigned short port, unsigned int addr);
int ipc_accept(int server_fd);
int ipc_connect(unsigned short port, unsigned int addr);
int ipc_write(int fd, void *buf, size_t count);
int ipc_read(int fd, void *buf, size_t count);
void aei_send_ipc(unsigned short port, Tr69MonitorMsgBody *msg);

#endif
