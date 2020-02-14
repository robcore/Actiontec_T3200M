/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <syslog.h>
#include <linux/if.h>
#include <sys/stat.h>
#include "dproxy.h"
#include "dns_decode.h"
#include "cache.h"
#include "conf.h"
#include "dns_list.h"
#include "dns_construct.h"
#include "dns_io.h"
#include "dns_dyn_cache.h"
#include "dns_mapping.h"
#include "prctl.h"

/*****************************************************************************/
/*****************************************************************************/
int dns_main_quit;
int dns_sock[2]={-1, -1};
fd_set rfds;

#if defined(DMP_X_BROADCOM_COM_IPV6_1) || defined(GPL_CODE_CONTROL_LAYER) /*added by libby 2012-04-25*/
char *name_server_ipv6;
#endif

//BRCM
int dns_querysock[2]={-1, -1};
#ifdef DMP_X_ITU_ORG_GPON_1
static unsigned int dns_query_error = 0;
#endif

/* CMS message handle */
void *msgHandle=NULL;
int msg_fd;

#ifdef GPL_CODE_FRONTIER_V2200H
static int isWebsiteBlock(char *dnsName,char *dnsIp);
static void get_lanip (char *ip, int len);
#endif

#if defined(GPL_CODE_DNS_CACHE)
time_t syn_time=0;
int enable_dns_cache=0;

int dns_cache_enabled()
{
   time_t t=time(NULL);
   if (t-syn_time>3)
   {
	syn_time=t;
	struct stat stats;
	if(stat("/var/dns_cache", &stats) == 0)
	  enable_dns_cache=1;
	else
	  enable_dns_cache=0;
   }
   return enable_dns_cache;

}
#endif

#if defined(GPL_CODE_FRONTIER_WIZARD)

/*
 * Function: AEI_get_ipandnetmask()
 *
 * Description: This function can get the lan ip and netmask
 *
 * Input Parameters:        ip       --> the DUT getway ip address.
 *                          netmask  --> the DUT getway netmask.
 *                          len --> the length of the ip
 *
 * Output Parameters:       netmask  --> the DUT getway netmask.
 *                          ip       --> the DUT getway ip address.
 *
 * Return Values: None
 *
 * Usage:
 *
 */
static void AEI_get_ipandnetmask (char *ip,char *netmask, int len)
{
    FILE *fp = NULL;
    fp = fopen("/var/laninfo","r");
    if(!fp)
        return;
    fscanf(fp, "%s %s\n", ip, netmask);
    fclose(fp);
}


#endif


/*****************************************************************************/
int dns_init()
{
  struct addrinfo hints, *res, *p;
  int errcode;
  int ret, i, on=1;

  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  /*
   * BRCM:
   * Open sockets to receive DNS queries from LAN.
   * dns_sock[0] is used for DNS queries over IPv4
   * dns_sock[1] is used for DNS queries over IPv6
   */
  errcode = getaddrinfo(NULL, PORT_STR, &hints, &res);
  if (errcode != 0)
  {
     debug("gai err %d %s", errcode, gai_strerror(errcode));
     exit(1) ;
  }

  p = res;
  while (p)
  {
     if ( p->ai_family == AF_INET )   i = 0;
     else if ( p->ai_family == AF_INET6 )   i = 1;
     else {
        debug_perror("Unknown protocol!!\n");
        goto next_lan;
     }

     dns_sock[i] = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

     if (dns_sock[i] < 0)
     {
        debug("Could not create dns_sock[%d]", i);
        goto next_lan;
     }

#ifdef IPV6_V6ONLY
     if ( (p->ai_family == AF_INET6) && 
          (setsockopt(dns_sock[i], IPPROTO_IPV6, IPV6_V6ONLY, 
                      &on, sizeof(on)) < 0) )
     {
        debug_perror("Could not set IPv6 only option for LAN");
        close(dns_sock[i]);
        goto next_lan;
     }
#endif

     /* bind() the socket to the interface */
     if (bind(dns_sock[i], p->ai_addr, p->ai_addrlen) < 0){
        debug("dns_init: bind: dns_sock[%d] can't bind to port", i);
        close(dns_sock[i]);
     }

next_lan:
     p = p->ai_next;
  }

  freeaddrinfo(res);

  if ( (dns_sock[0] < 0) && (dns_sock[1] < 0) )
  {
     debug_perror("Cannot create sockets for LAN");
     exit(1) ;
  }

  /*
   * BRCM:
   * use different sockets to send queries to WAN so we can use ephemeral port
   * dns_querysock[0] is used for DNS queries sent over IPv4
   * dns_querysock[1] is used for DNS queries sent over IPv6
   */
  errcode = getaddrinfo(NULL, "0", &hints, &res);
  if (errcode != 0)
  {
     debug("gai err %d %s", errcode, gai_strerror(errcode));
     exit(1) ;
  }

  p = res;
  while (p)
  {
     if ( p->ai_family == AF_INET )   i = 0;
     else if ( p->ai_family == AF_INET6 )   i = 1;
     else
     {
        debug_perror("Unknown protocol!!\n");
        goto next_wan;
     }

     dns_querysock[i] = socket(p->ai_family, p->ai_socktype, 
                               p->ai_protocol);

     if (dns_querysock[i] < 0)
     {
        debug("Could not create dns_querysock[%d]", i);
        goto next_wan;
     }

#ifdef IPV6_V6ONLY
     if ( (p->ai_family == AF_INET6) && 
          (setsockopt(dns_querysock[i], IPPROTO_IPV6, IPV6_V6ONLY, 
                     &on, sizeof(on)) < 0) )
     {
        debug_perror("Could not set IPv6 only option for WAN");
        close(dns_querysock[i]);
        goto next_wan;
     }
#endif

     /* bind() the socket to the interface */
     if (bind(dns_querysock[i], p->ai_addr, p->ai_addrlen) < 0){
        debug("dns_init: bind: dns_querysock[%d] can't bind to port", i);
        close(dns_querysock[i]);
     }

next_wan:
     p = p->ai_next;
  }

  freeaddrinfo(res);

  if ( (dns_querysock[0] < 0) && (dns_querysock[1] < 0) )
  {
     debug_perror("Cannot create sockets for WAN");
     exit(1) ;
  }

  // BRCM: Connect to ssk
  if ((ret = cmsMsg_initWithFlags(EID_DNSPROXY, 0, &msgHandle)) != CMSRET_SUCCESS) {
  	debug_perror("dns_init: cmsMsg_init() failed");
	exit(1);
  }
  cmsMsg_getEventHandle(msgHandle, &msg_fd);

  dns_main_quit = 0;

  FD_ZERO( &rfds );
  if (dns_sock[0] > 0)
     FD_SET( dns_sock[0], &rfds );
  if (dns_sock[1] > 0)  
     FD_SET( dns_sock[1], &rfds );
  if (dns_querysock[0] > 0)  
     FD_SET( dns_querysock[0], &rfds );
  if (dns_querysock[1] > 0)  
     FD_SET( dns_querysock[1], &rfds );

  FD_SET( msg_fd, &rfds );

#ifdef DNS_DYN_CACHE
  dns_dyn_hosts_add();
#endif

#if defined(GPL_CODE_DNS_CACHE)
  cache_purge( config.purge_time );
#endif

  return 1;
}


/*****************************************************************************/
void dns_handle_new_query(dns_request_t *m)
{
   int retval = -1;
   dns_dyn_list_t *dns_entry;
   char srcIP[INET6_ADDRSTRLEN];
   int query_type = m->message.question[0].type;

   if( query_type == A )
   {
      retval = 0;         
#ifdef DNS_DYN_CACHE
#if defined(GPL_CODE_DNS_CACHE)
    retval = cache_lookup_name( m->cname, m->ip );
#ifdef GPL_CODE_CONTROL_LAYER
    if (0==retval)
    {
        char name_app[128] = {0};
        snprintf( name_app, sizeof(name_app)-1, "[%s]", m->cname );
        retval = cache_lookup_name( name_app, m->ip );
        if (retval==1)
            retval= 5;
    }
#endif

    if(0 == retval)
    {
        if(!dns_cache_enabled())
            retval = 0;
        else
        {
#endif
      if ((dns_entry = dns_dyn_find(m))) 
      {
#if defined(GPL_CODE_COVERITY_FIX)
          /*CID 12232: Copy into fixed size buffer*/
          strlcpy(m->ip, inet_ntoa(dns_entry->addr), sizeof(m->ip));
#else
         strcpy(m->ip, inet_ntoa(dns_entry->addr));
#endif
         m->ttl = abs(dns_entry->expiry - time(NULL));
         retval = 1;
#ifndef GPL_CODE_CONTROL_LAYER
         debug(".... %s, srcPort=%d, retval=%d\n", m->cname, ntohs(((struct sockaddr_in *)&m->src_info)->sin_port), retval); 
#endif
      }
#if defined(GPL_CODE_DNS_CACHE)
        }
    }
#endif
#endif      
   }
   else if( query_type == AAA)
   { /* IPv6 todo */
#if defined(GPL_CODE_CONTROL_LAYER)
		/*modify by libby support IPV6*/
#ifdef GPL_CODE_DNS_CACHE
		char name_app[128] = {0};
		snprintf( name_app, sizeof(name_app)-1, "[%s]", m->cname );
		retval = cache_lookup_name( name_app, m->ip );
		//strcpy(m->cname,name_app);
		if( 1 != retval )
#endif
#endif

// To support query to WAN IPv6 DNS server (retval == 6),
// must implement CMS_MSG_DNSPROXY_IPV6_CHANGED to notice IPv6 DNS server info change.
#if defined(GPL_CODE_CONTROL_LAYER)
		{
			debug(" remote ipv6 dns server address is %s\n", name_server_ipv6 );
			if(name_server_ipv6)
			{
				retval = 6;
			}
			else
			{
				retval = 0;
			}
		}
#else
     retval = 0;
#endif
   }
   else if( query_type == PTR ) 
   {
      retval = 0;   
      /* reverse lookup */
#ifdef DNS_DYN_CACHE
#ifdef GPL_CODE_DNS_CACHE
        if(!dns_cache_enabled())
            retval = 0;
        else
        {
#endif
      if ((dns_entry = dns_dyn_find(m))) 
      {
#if defined(GPL_CODE_COVERITY_FIX)
          /*CID 12232: Copy into fixed size buffer*/
          strlcpy(m->cname, dns_entry->cname, sizeof(m->cname));
#else
         strcpy(m->cname, dns_entry->cname);
#endif
         m->ttl = abs(dns_entry->expiry - time(NULL));
         retval = 1;
#ifndef GPL_CODE_CONTROL_LAYER
         debug("TYPE==PTR.... %s, srcPort=%d, retval=%d\n", m->cname,  ntohs(((struct sockaddr_in *)&m->src_info)->sin_port), retval); 
#endif
         
      }
#ifdef GPL_CODE_DNS_CACHE
        }
#endif
#endif
   }  
   else /* BRCM:  For all other query_type including TXT, SPF... */
   {
       retval = 0;
   }
   
   inet_ntop( m->src_info.ss_family, 
              get_in_addr(&m->src_info), 
              srcIP, INET6_ADDRSTRLEN );
   cmsLog_notice("dns_handle_new_query: %s %s, srcPort=%d, retval=%d\n", 
                  m->cname, srcIP, 
                  ntohs(((struct sockaddr_in *)&m->src_info)->sin_port), 
                  retval);
   
  switch( retval )
  {
     case 0:
     {
        char dns1[INET6_ADDRSTRLEN];
        int proto;

#ifdef GPL_CODE
        if (m->message.header.flags.f.opcode == STATUS)
        {
            cmsLog_debug("The status request would be dropped.");
        }
        else if (dns_mapping_find_dns_ip(&(m->src_info), query_type, dns1, &proto) == TRUE)
#else
        if (dns_mapping_find_dns_ip(&(m->src_info), query_type, 
                                    dns1, &proto) == TRUE)
#endif
        {
           struct sockaddr_storage sa;
           int sock;

           cmsLog_notice("Found dns %s for subnet %s", dns1, srcIP);
           dns_list_add(m);

           if (proto == AF_INET)
           {
              sock = dns_querysock[0];
              ((struct sockaddr_in *)&sa)->sin_port = htons(PORT);
              inet_pton(proto, dns1, &(((struct sockaddr_in *)&sa)->sin_addr));
           }
           else
           {
              sock = dns_querysock[1];
              ((struct sockaddr_in6 *)&sa)->sin6_port = htons(PORT);
              inet_pton( proto, dns1, 
                         &(((struct sockaddr_in6 *)&sa)->sin6_addr) );
           }
           sa.ss_family = proto;
           dns_write_packet( sock, &sa, m );
        }
        else
        {
#if defined (AEI_FRONTIER_WIZARD)
            if(access("/var/wizardPortalFlag",R_OK)==0 && access("/var/iptablesportal",R_OK)==0)
            {
                char lan_ip[BUFLEN_64]={0};
                char netmask[BUFLEN_64]={0};
                memset (lan_ip, 0, sizeof(lan_ip));

                AEI_get_ipandnetmask(lan_ip,netmask, sizeof(lan_ip));

                if (strlen (lan_ip) == 0)
                    strcpy(lan_ip,"192.168.254.254");
                memset(dns1,'\0',INET6_ADDRSTRLEN);
                inet_ntop( AF_INET,get_in_addr(&m->src_info),dns1, INET6_ADDRSTRLEN );
                strncpy(m->ip,lan_ip,sizeof(m->ip)-1);
                inet_pton(AF_INET, dns1, &(((struct sockaddr_in *)&m->src_info)->sin_addr));
                m->src_info.ss_family = AF_INET;
                dns_construct_reply( m );
                dns_write_packet( dns_sock[0], &m->src_info, m );
            }else
#endif
            cmsLog_debug("No dns service.");
        }   
        break;
     }

     case 1:
        dns_construct_reply( m );
        /* Only IPv4 can support caching now */
        dns_write_packet(dns_sock[0], &m->src_info, m);
        break;

     default:
        debug("Unknown query type: %d\n", query_type);
   }
}
/*****************************************************************************/
void dns_handle_request(dns_request_t *m)
{
  dns_request_t *ptr = NULL;

  /* request may be a new query or a answer from the upstream server */
  ptr = dns_list_find_by_id(m);

  if( ptr != NULL ){
      debug("Found query in list; id 0x%04x flags 0x%04x\n ip %s - cname %s\n", 
             m->message.header.id, m->message.header.flags.flags, 
             m->ip, m->cname);

      /* message may be a response */
      if( m->message.header.flags.flags & 0x8000)
      {
          int sock;
          char srcIP[INET6_ADDRSTRLEN];

          if (ptr->src_info.ss_family == AF_INET)
             sock = dns_sock[0];
          else
             sock = dns_sock[1];

          inet_ntop( m->src_info.ss_family, 
                     get_in_addr(&m->src_info), 
                     srcIP, INET6_ADDRSTRLEN );
   
          debug("Replying with answer from %s, id 0x%04x\n", 
                 srcIP, m->message.header.id);
#if defined (AEI_FRONTIER_WIZARD)
            if(access("/var/wizardPortalFlag",R_OK)==0 && access("/var/iptablesportal",R_OK)==0)
            {
                char lan_ip[BUFLEN_64]={0};
                char netmask[BUFLEN_64]={0};
                memset (lan_ip, 0, sizeof(lan_ip));

                AEI_get_ipandnetmask(lan_ip,netmask, sizeof(lan_ip));

                if (strlen (lan_ip) == 0)
                    strcpy(lan_ip,"192.168.254.254");
                strncpy(m->ip,lan_ip,sizeof(m->ip)-1);
            }
#endif
          dns_write_packet( sock, &ptr->src_info, m );

          /* Only remove the dns query when the retx_count is zero */
          if( !ptr->retx_count )
          	dns_list_remove( ptr );
          else
          	ptr->retx_count--;
          
#ifdef DMP_X_ITU_ORG_GPON_1
          if( m->message.header.flags.f.rcode != 0 ){ // lookup failed
              dns_query_error++;
              debug("dns_query_error = %u\n", dns_query_error);
          }
#endif

#ifdef DNS_DYN_CACHE
#ifdef GPL_CODE_DNS_CACHE
      if(dns_cache_enabled())
#endif
          if( m->message.question[0].type != AAA) /* No cache for IPv6 */
          {
             dns_dyn_cache_add(m);
          }
#endif
      }
      else
      {
         debug( "Duplicate query to %s (retx_count=%d)", 
                m->cname, ptr->retx_count );

         if (ptr->retx_count < MAX_RETX_COUNT)
         {
            char dns1[INET6_ADDRSTRLEN];
            int proto;

            debug("=>send again\n");
            ptr->retx_count++;
            cmsLog_debug("retx_count=%d", ptr->retx_count);

            if (dns_mapping_find_dns_ip(&(m->src_info), 
                                        m->message.question[0].type, 
                                        dns1, &proto) == TRUE)
            {
               struct sockaddr_storage sa;
               int sock;

               if (proto == AF_INET)
               {
                  sock = dns_querysock[0];
#if defined(GPL_CODE)
                  ((struct sockaddr_in *)&sa)->sin_port = htons(PORT);
#else
                  ((struct sockaddr_in *)&sa)->sin_port = PORT;
#endif
                  inet_pton( proto, dns1, 
                             &(((struct sockaddr_in *)&sa)->sin_addr) );
               }
               else
               {
                  sock = dns_querysock[1];
#if defined(GPL_CODE)
                  ((struct sockaddr_in6 *)&sa)->sin6_port = htons(PORT);
#else
                  ((struct sockaddr_in6 *)&sa)->sin6_port = PORT;
#endif
                  inet_pton( proto, dns1, 
                             &(((struct sockaddr_in6 *)&sa)->sin6_addr) );
               }
               sa.ss_family = proto;
               dns_write_packet( sock, &sa, m );
            }
            else
            {
               cmsLog_debug("No dns service for duplicate query??");
            }   
         }
         else
         {
            debug("=>drop! retx limit reached.\n");
         }
      }
  }
  else
  {
      dns_handle_new_query( m );
  }

  debug("dns_handle_request: done\n\n");
}

/*****************************************************************************/
static void processCmsMsg(void)
{
#ifdef GPL_CODE_COVERITY_FIX
   /*Coverity CID 12493: Use of untrusted scalar value (TAINTED_SCALAR)*/
   CmsMsgHeader *msg = NULL;
#else
  CmsMsgHeader *msg;
#endif
  CmsRet ret;

  while( (ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, 0)) == CMSRET_SUCCESS) {
    switch(msg->type) {
    case CMS_MSG_SET_LOG_LEVEL:
      cmsLog_debug("got set log level to %d", msg->wordData);
      cmsLog_setLevel(msg->wordData);
      if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS) {
        cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
      }
      break;
    case CMS_MSG_SET_LOG_DESTINATION:
      cmsLog_debug("got set log destination to %d", msg->wordData);
      cmsLog_setDestination(msg->wordData);
      if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS) {
        cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
      }
      break;

    case CMS_MSG_DNSPROXY_RELOAD:
      cmsLog_debug("received CMS_MSG_DNSPROXY_RELOAD\n");
      /* Reload config file */
#ifdef DNS_DYN_CACHE
      dns_dyn_hosts_add();
#if defined(GPL_CODE_DNS_CACHE)
      cache_purge( config.purge_time );
#endif
#endif

      /* load the /var/dnsinfo.conf into the linked list for determining
      * which dns ip to use for the dns query.
      */
      dns_mapping_conifg_init();
     
      /*
       * During CDRouter dhcp scaling tests, this message is sent a lot to dnsproxy.
       * To make things more efficient/light weight, the sender of the message does
       * not expect a reply.
       */
      break;

#ifdef SUPPORT_DEBUG_TOOLS

    case CMS_MSG_DNSPROXY_DUMP_STATUS:
       printf("\n============= Dump dnsproxy status=====\n\n");
       printf("WAN interface; LAN IP/MASK; Primary DNS IP,Secondary DNS IP;\n");
       prctl_runCommandInShellBlocking("cat /var/dnsinfo.conf");
       dns_list_print();
       dns_dyn_print();
       break;

    case CMS_MSG_DNSPROXY_DUMP_STATS:
       printf("stats dump not implemented yet\n");
       break;

#endif /* SUPPORT_DEBUG_TOOLS */

#ifdef DMP_X_ITU_ORG_GPON_1
    case CMS_MSG_DNSPROXY_GET_STATS:
    {
        char buf[sizeof(CmsMsgHeader) + sizeof(DnsGetStatsMsgBody)]={0};
        CmsMsgHeader *replyMsgPtr = (CmsMsgHeader *) buf;
        DnsGetStatsMsgBody *dnsStats = (DnsGetStatsMsgBody *) (replyMsgPtr+1);


        // Setup response message.
        replyMsgPtr->type = msg->type;
        replyMsgPtr->dst = msg->src;
        replyMsgPtr->src = EID_DNSPROXY;
        replyMsgPtr->flags.all = 0;
        replyMsgPtr->flags_response = 1;
        //set dns query error counter 
        replyMsgPtr->dataLength = sizeof(DnsGetStatsMsgBody);
        dnsStats->dnsErrors = dns_query_error;

        cmsLog_notice("dns query error is %d", dns_query_error);
        // Attempt to send CMS response message & test result.
        ret = cmsMsg_send(msgHandle, replyMsgPtr);
        if (ret != CMSRET_SUCCESS)
        {
           // Log error.
           cmsLog_error("Send message failure, cmsReturn: %d", ret);
        }
    }
    break;
#endif

    default:
      cmsLog_error("unrecognized msg 0x%x", msg->type);
      break;
    }
    CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
  }
  
  if (ret == CMSRET_DISCONNECTED) {
    if (!cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS)) {
      cmsLog_error("lost connection to smd, exiting now.");
    }
    dns_main_quit = 1;
  }
}

/*****************************************************************************/
int dns_main_loop()
{
    struct timeval tv, *ptv;
    fd_set active_rfds;
    int retval;
    dns_request_t m;
    dns_request_t *ptr, *next;
#if defined(GPL_CODE_DNS_CACHE)
    int purge_time = PURGE_TIMEOUT;
#endif

    while( !dns_main_quit )
    {

      int next_request_time = dns_list_next_time();
      
      if (next_request_time == 1) {
          tv.tv_sec = next_request_time;
          tv.tv_usec = 0;
          ptv = &tv;
          cmsLog_notice("select timeout = %lu seconds", tv.tv_sec);          
       } else {
         ptv = NULL;
         cmsLog_debug("\n\n =============select will wait indefinitely============");          
       }


      /* copy the main rfds in the active one as it gets modified by select*/
      active_rfds = rfds;
      retval = select( FD_SETSIZE, &active_rfds, NULL, NULL, ptv );

      if (retval){
         debug("received something");

         if (FD_ISSET(msg_fd, &active_rfds)) { /* message from ssk */
            debug("received cms message");
            processCmsMsg();
         } else if ((dns_sock[0] > 0) && FD_ISSET(dns_sock[0], &active_rfds)) {
            debug("received DNS message (LAN side IPv4)");
            /* data is now available */
            bzero(&m, sizeof(dns_request_t));
            //BRCM                
            if (dns_read_packet(dns_sock[0], &m) == 0) {
/*
 * For frontier requirement,website block use dns.so if the dns package from lan side ,then 
 * check the request hostname whether is in the block url list, if so ,construct a response package with 
 * ip with lan ip, and src addr with lan ip to the request's client  
 */
#ifdef GPL_CODE_FRONTIER_V2200H               
               char ipaddr[64]={0};
               inet_ntop( m.src_info.ss_family,get_in_addr(&m.src_info),ipaddr, INET6_ADDRSTRLEN );               
               //strcpy(ipaddr,inet_ntoa(m.src_addr)); 
               if(isWebsiteBlock(m.cname,ipaddr))
               {
                  char lan_ip[64]={0};
                  memset (lan_ip, 0, sizeof(lan_ip));
                  get_lanip(lan_ip, sizeof(lan_ip)-1);
                  if (strlen (lan_ip) == 0)
                      strcpy(lan_ip,"192.168.254.254");
                  strncpy(m.ip,lan_ip,sizeof(m.ip)-1);
                  inet_pton(AF_INET, ipaddr, &(((struct sockaddr_in *)&m.src_info)->sin_addr));
                  m.src_info.ss_family = AF_INET;
                  dns_construct_reply( &m );
                  dns_write_packet( dns_sock[0], &m.src_info, &m );            
               }else
#endif   
               dns_handle_request( &m );
            }
         } else if ((dns_sock[1] > 0) && FD_ISSET(dns_sock[1], &active_rfds)) {
            debug("received DNS message (LAN side IPv6)");
            /* data is now available */
            bzero(&m, sizeof(dns_request_t));
            //BRCM
            if (dns_read_packet(dns_sock[1], &m) == 0) {
               dns_handle_request( &m );
            }
         } else if ((dns_querysock[0] > 0) && 
                    FD_ISSET(dns_querysock[0], &active_rfds)) {
            debug("received DNS response (WAN side IPv4)");
            bzero(&m, sizeof(dns_request_t));
            if (dns_read_packet(dns_querysock[0], &m) == 0)
               dns_handle_request( &m );
         } else if ((dns_querysock[1] > 0) && 
                    FD_ISSET(dns_querysock[1], &active_rfds)) {
            debug("received DNS response (WAN side IPv6)");
            bzero(&m, sizeof(dns_request_t));
            if (dns_read_packet(dns_querysock[1], &m) == 0)
               dns_handle_request( &m );
         }
      } else { /* select time out */
         time_t now = time(NULL);
         ptr = dns_request_list;
         while (ptr) {
            next = ptr->next;
            if (ptr->expiry <= now) {
               char date_time_buf[DATE_TIME_BUFLEN];
               get_date_time_str(date_time_buf, sizeof(date_time_buf));

               debug("removing expired request %p\n", ptr);
               cmsLog_notice("%s dnsproxy: query for %s timed out after %d secs (type=%d retx_count=%d)",  
                  date_time_buf, ptr->cname, DNS_TIMEOUT, (unsigned int) ptr->message.question[0].type, ptr->retx_count);

               /*  dns1 and dns2 will be swapped if possible in dns_list_remove_related_requests_and_swap call */
               if (dns_list_remove_related_requests_and_swap(ptr)) {
                  /* reset to the header since dns_list_remove_related_requests_and_swap may free the dns requests with 
                  * the using the same dns ip
                  */
                  ptr = dns_request_list;
                  continue;
               }
               
            }

            ptr = next;
         }

      } /* if (retval) */

#if defined(GPL_CODE_DNS_CACHE)
      purge_time--;
      if( !purge_time ){
         cache_purge( config.purge_time );
         purge_time = PURGE_TIMEOUT;
      }
#endif
    }  /* while (!dns_main_quit) */
   return 0;
}


/*
 * Return a buffer which contains the current date/time.
 */
void get_date_time_str(char *buf, unsigned int buflen)
{
	time_t t;
	struct tm *tmp;

	memset(buf, 0, buflen);

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
		debug_perror("could not get localtime");
		return;
	}

	strftime(buf, buflen, "[%F %k:%M:%S]", tmp);

}

#ifdef GPL_CODE_FRONTIER_V2200H
/*
 * if find out the hostname and ip match the url list ,then return 1, else return 0
 */
#define MAX_WEB_LEN	40
#define MAX_FOLDER_LEN	56
static int isWebsiteBlock(char *dnsName,char *dnsIp)
{
    char temp[MAX_WEB_LEN + MAX_FOLDER_LEN], *temp1, *temp2, web[MAX_WEB_LEN], folder[MAX_FOLDER_LEN];
#if defined(DMP_X_BROADCOM_COM_IPV6_1) || defined(GPL_CODE_CONTROL_LAYER)
	char *Ip=NULL;		
#endif    
	FILE *f = fopen("/var/url_list", "r");
	if (f != NULL){
	   while (fgets(temp,96, f))
	   {
           char lanIP[16] = { 0 };
           char *pos = NULL;
           char *pe = NULL;
           if ((pos = strchr(temp, ';')) != NULL) {
               *pos++ = '\0';
               if (*pos == '\0')
                   strcpy(lanIP, "\0");
               else {
                   pe = strrchr(pos, '\n');
                   if(pe)
                       *pe = '\0';
                }
#if defined(GPL_CODE_COVERITY_FIX)
               //CID 33160: Copy into fixed size buffer(STRING_OVERFLOW)
               strlcpy(lanIP, pos, sizeof(lanIP));
#else               
               strcpy(lanIP, pos);
#endif                
           }
            
		   if (temp[0]=='h' && temp[1]=='t' && temp[2]=='t' && 
		    	temp[3]=='p' && temp[4]==':' && temp[5]=='/' && temp[6]=='/')
		   {
			   temp1 = temp + 7;	
		   }
		   else
		   {
		       temp1 = temp;	
		   }

		   if ((*temp1=='w') && (*(temp1+1)=='w') && (*(temp1+2)=='w') && (*(temp1+3)=='.'))
		   {
			   temp1 = temp1 + 4;
		   }

		   if ((temp2 = strchr(temp1, '\n')))
		   {
			   *temp2 = '\0';
		   }
		       
		   sscanf(temp1, "%[^/]", web);	
		   temp1 = strchr(temp1, '/');
		   if (temp1 == NULL)
		   {
			   strcpy(folder, "\0");
		   }
		   else
		   {
			   strcpy(folder, ++temp1);		
		   }           
//if defined ipv6, then the ip contain prefix "::ffff:", so need parse out the ipv4 address           
#if defined(DMP_X_BROADCOM_COM_IPV6_1) || defined(GPL_CODE_CONTROL_LAYER)           
          Ip = strtok(dnsIp,":");
          if(Ip==NULL)
              break;
          while(Ip!=NULL)
          {
              Ip=strtok(NULL,":");
          }
#endif          
          if(cmsUtl_strstr(dnsName,web)!=NULL)
          {
#if defined(DMP_X_BROADCOM_COM_IPV6_1) || defined(GPL_CODE_CONTROL_LAYER)              
              if(cmsUtl_strstr(lanIP,Ip)!=NULL)
#else
              if(cmsUtl_strstr(lanIP,dnsIp)!=NULL)
#endif                  
              {
                  fclose(f);
                  return 1;
              }
              if(cmsUtl_strstr(lanIP,"all")!=NULL)
              {
                  fclose(f);
                  return 1;
              }
          }
	   }
	   fclose(f);
	}
	else {
	   debug("/var/url_list isn't presented.\n");
	   return 0;
	}
	return 0;
}

static void get_lanip (char *ip, int len)
{
  FILE *fp;
  char buffer[BUFLEN_256] = {0};
  char host_name[BUFLEN_128] = {0};
  char ip_address[BUFLEN_32] = {0};
  int  i =0;

  fp = fopen("/var/hosts", "r");
  if (!fp)
     return;

  memset (buffer, 0, sizeof(buffer));
  while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) 
  {  
     i++;
     memset(ip_address, 0, sizeof(ip_address));
     memset(host_name, 0, sizeof(host_name));    
     sscanf(buffer, "%s %s", ip_address, host_name);
     if (i == 2)
     {
         if ( (strlen (host_name) > 0) && (strlen(ip_address) > 0) )
         {
             strncpy (ip, ip_address, len);
         }
         break;
     }
     memset (buffer, 0, sizeof(buffer));
  }
  fclose (fp);
  return;
}

#endif
/*****************************************************************************/
void debug_perror( char * msg ) {
	debug( "%s : %s\n" , msg , strerror(errno) );
}

#if 0 //BRCM: Mask the debug() function. It's redefined by cmsLog_debug()
#ifdef DPROXY_DEBUG
/*****************************************************************************/
void debug(char *fmt, ...)
{
#define MAX_MESG_LEN 1024
  
  va_list args;
  char text[ MAX_MESG_LEN ];
  
  sprintf( text, "[ %d ]: ", getpid());
  va_start (args, fmt);
  vsnprintf( &text[strlen(text)], MAX_MESG_LEN - strlen(text), fmt, args);	 
  va_end (args);
  
  printf(text);
#if 0 //BRCM 
  if( config.debug_file[0] ){
	 FILE *fp;
	 fp = fopen( config.debug_file, "a");
	 if(!fp){
		syslog( LOG_ERR, "could not open log file %m" );
		return;
	 }
	 fprintf( fp, "%s", text);
	 fclose(fp);
  }
  

  /** if not in daemon-mode stderr was not closed, use it. */
  if( ! config.daemon_mode ) {
	 fprintf( stderr, "%s", text);
  }
#endif
}

#endif
#endif
/*****************************************************************************
 * print usage informations to stderr.
 * 
 *****************************************************************************/
void usage(char * program , char * message ) {
  fprintf(stderr,"%s\n" , message );
  fprintf(stderr,"usage : %s [-c <config-file>] [-d] [-h] [-P]\n", program );
  fprintf(stderr,"\t-c <config-file>\tread configuration from <config-file>\n");
  fprintf(stderr,"\t-d \t\trun in debug (=non-daemon) mode.\n");
  fprintf(stderr,"\t-D \t\tDomain Name\n");
  fprintf(stderr,"\t-P \t\tprint configuration on stdout and exit.\n");
  fprintf(stderr,"\t-v \t\tset verbosity, where num==0 is LOG_ERROR, 1 is LOG_NOTICE, all others is LOG_DEBUG\n");
  fprintf(stderr,"\t-h \t\tthis message.\n");
}
/*****************************************************************************
 * get commandline options.
 * 
 * @return 0 on success, < 0 on error.
 *****************************************************************************/
int get_options( int argc, char ** argv ) 
{
  int c = 0;
  char * progname = argv[0];
  SINT32 logLevelNum;
  CmsLogLevel logLevel=DEFAULT_LOG_LEVEL;
  //UBOOL8 useConfiguredLogLevel=TRUE;

  cmsLog_initWithName(EID_DNSPROXY, argv[0]);

  conf_defaults();
#if 1 
  while( (c = getopt( argc, argv, "cD:dhPv:")) != EOF ) {
    switch(c) {
	 case 'c':
  		conf_load(optarg);
		break;
	 case 'd':
		break;
     case 'D':
#if defined(GPL_CODE_COVERITY_FIX)
         /*CID 12233: Copy into fixed size buffer*/
         strlcpy(config.domain_name, optarg, sizeof(config.domain_name));
#else
        strcpy(config.domain_name, optarg);
#endif
        break;
	 case 'h':
		usage(progname,"");
		return -1;
	 case 'P':
		break;
         case 'v':
         	logLevelNum = atoi(optarg);
         	if (logLevelNum == 0)
         	{
            		logLevel = LOG_LEVEL_ERR;
         	}
         	else if (logLevelNum == 1)
         	{
            		logLevel = LOG_LEVEL_NOTICE;
         	}
         	else
         	{
            		logLevel = LOG_LEVEL_DEBUG;
         	}
         	cmsLog_setLevel(logLevel);
         	//useConfiguredLogLevel = FALSE;
         	break;
	 default:
		usage(progname,"");
		return -1;
    }
  }
#endif

#if 0  
  /** unset daemon-mode if -d was given. */
  if( not_daemon ) {
	 config.daemon_mode = 0;
  }
  
  if( want_printout ) {
	 conf_print();
	 exit(0);
  }
#endif 

  return 0;
}
/*****************************************************************************/
void sig_hup (int signo)
{
  signal(SIGHUP, sig_hup); /* set this for the next sighup */
  conf_load (config.config_file);
}
/*****************************************************************************/
int main(int argc, char **argv)
{

  /* get commandline options, load config if needed. */
  if(get_options( argc, argv ) < 0 ) {
	  exit(1);
  }

  /* detach from terminal and detach from smd session group. */
  if (setsid() < 0)
  {
    cmsLog_error("could not detach from terminal");
    exit(-1);
  }

  /* ignore some common, problematic signals */
  signal(SIGINT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  signal(SIGHUP, sig_hup);
  dns_init();

//BRCM: Don't fork a task again!
#if 0 
  if (config.daemon_mode) {
    /* Standard fork and background code */
    switch (fork()) {
	 case -1:	/* Oh shit, something went wrong */
		debug_perror("fork");
		exit(-1);
	 case 0:	/* Child: close off stdout, stdin and stderr */
		close(0);
		close(1);
		close(2);
		break;
	 default:	/* Parent: Just exit */
		exit(0);
    }
  }
#endif
  dns_main_loop();

  return 0;
}

