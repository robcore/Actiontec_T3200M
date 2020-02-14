#ifndef _AEI_NF_CONNTRACK_ALG_QOS_H
#define _AEI_NF_CONNTRACK_ALG_QOS_H

/* the following defines must keep the same with those in userspace file "aei_ssk.h" */

#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]

typedef enum
{
    ALG_QOS_ACTION_NONE = 0,
    ALG_QOS_ACTION_ADD_CONN,
    ALG_QOS_ACTION_DEL_CONN,
} alg_qos_action_e;

typedef enum
{
    ALG_QOS_PROTO_NONE = 0,
    ALG_QOS_PROTO_SIP,
    ALG_QOS_PROTO_H323,
} alg_qos_proto_e;

typedef struct alg_qos_msg
{
    alg_qos_action_e action;
    alg_qos_proto_e proto;
    uint32_t ip_proto;
    uint32_t src_ip;
    uint32_t src_port;
    uint32_t dest_ip;
    uint32_t dest_port;
} alg_qos_msg_t;

void aei_alg_qos_handle_conn(alg_qos_action_e action, alg_qos_proto_e proto, struct nf_conntrack_tuple *tuple); 

#endif /* _AEI_NF_CONNTRACK_ALG_QOS_H */
