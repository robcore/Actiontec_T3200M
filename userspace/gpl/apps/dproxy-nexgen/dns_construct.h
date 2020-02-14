#include "dproxy.h"

void dns_construct_reply( dns_request_t *m );
void dns_construct_error_reply(dns_request_t *m);
#ifdef GPL_CODE_CONTROL_LAYER
void dns_construct_reject_reply(dns_request_t * m);
#endif
