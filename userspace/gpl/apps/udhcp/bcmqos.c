/***********************************************************************
 * <:copyright-BRCM:2010:DUAL/GPL:standard
 * 
 *    Copyright (c) 2010 Broadcom Corporation
 *    All Rights Reserved
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * 
 * A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
 * writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * 
 * :>
************************************************************************/
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "dhcpd.h"
#include "cms_log.h"

#include "cms_msg.h"
#include "packet.h"

#if defined(GPL_CODE)
#include "serverpacket.h"
#endif

/* Externs */
extern struct iface_config_t *iface_config;

typedef struct ExecIP {
   u_int32_t ipaddr;
   unsigned char execflag;
   u_int32_t index;
}EXECIP, PEXECIP;

typedef struct optioncmd {
   char command[1024];
   char action;
   int optionnum;
   char optionval[16];
   struct ExecIP execip[254];
#if defined(GPL_CODE_QOS)
    char MACAddresses[2048];
    char clsname[256];
#endif
   struct optioncmd *pnext;
}OPTIONCMD, POPTIONCMD;

struct optioncmd *optioncmdHead = NULL;

void bcmDelObsoleteRules(void);
void bcmExecOptCmd(void);
void bcmQosDhcp(int optionnum, char *cmd);

static char bcmParseCmdAction(char *cmd);
static void bcmSetQosRule(char action, char *command, u_int32_t leaseip);
static void bcmAddOptCmdIP(struct optioncmd * optcmd, u_int32_t leaseip, int index);
static struct optioncmd * bcmAddOptCmd(int optionnum, char action, char *cmd);
static void bcmDelOptCmd(char *cmd);

#ifdef GPL_CODE

int AEI_wstrcmp(const char *pat, const char *str)
{
    const char *p = NULL, *s = NULL;
    do {
        if (*pat == '*') {
            for (; *(pat + 1) == '*'; ++pat) ;
            p = pat++;
            s = str;
        }
        if (*pat == '?' && !*str)
            return -1;

        if (*pat != '?' && toupper(*pat) != toupper(*str)) {
            if (p == NULL)
                return -1;
            pat = p;
            str = s++;
        }
    } while (*pat && ++pat && *str++);
    for (; *pat == '*'; ++pat) ;
    return *pat;
}
#endif

#if defined(GPL_CODE_QOS)
//For debug purpose to print out the option cmd on the cmd linklist
void AEI_printOptionCmdList(void)
{
    struct optioncmd *pnode = optioncmdHead;
    int i = 1;

    printf("will print the option cmd linklist\r\n");
    while (pnode) {
        printf("option node %d\r\n", i);
        printf("clsname:%s, commmand:%s, optionval:%s, macs:%s\r\n", pnode->clsname, pnode->command, pnode->optionval,
               pnode->MACAddresses);
        pnode = pnode->pnext;
        i++;
    }
}

// dhcp send a message to smd, which includes all the mac address related to one option60 string
// vendorId will be used to differentiate different option60 related QoS classification
void AEI_send_macs_to_cms(const char *clsname, const char *macs, const char *option60String)
{
    char buf[sizeof(CmsMsgHeader) + sizeof(DhcpdHostInfoMsgBody)] = { 0 };

    CmsMsgHeader *hdr = (CmsMsgHeader *) buf;
    DhcpdHostInfoMsgBody *body = (DhcpdHostInfoMsgBody *) (hdr + 1);
    CmsRet ret;
    hdr->type = CMS_MSG_DHCPD_MACS_TO_CMS;
    hdr->src = EID_DHCPD;
    hdr->dst = EID_SSK;
    hdr->flags_event = 1;
    hdr->dataLength = sizeof(DhcpdHostInfoMsgBody);

    if (clsname)
        snprintf(body->clsname, sizeof(body->clsname), clsname);
    else
        memset(body->clsname, 0, sizeof(body->clsname));

    snprintf(body->macs, sizeof(body->macs), macs);

    if (option60String)
        snprintf(body->option60String, sizeof(body->option60String), option60String);
    else
        memset(body->option60String, 0, sizeof(body->option60String));

    if ((ret = cmsMsg_send(msgHandle, hdr)) != CMSRET_SUCCESS) {
        printf("could not send macs to cms!\n");
    } else {
        //printf("send macs to cms from dhcpd is ok!\n");
    }
}

int AEI_bcmQoscmdDelMac(struct optioncmd *optcmd, char *mac)
{
    char *ptokenstart;
    char cmdseg[256];
    char *command = optcmd->command;
    if (strlen(mac) < 12)
        return -1;
    strcpy(cmdseg, command);
    ptokenstart = strstr(cmdseg, "--ip-src");
    strcpy(ptokenstart, "--src ");
    strcat(ptokenstart, mac);
    //strcat(ptokenstart, " ");
    strcat(ptokenstart, strstr(command, "]") + 1);
#if 0
    ptokenstart = strstr(cmdseg, "-A");
    if (ptokenstart)
        *(ptokenstart + 1) = 'D';
    ptokenstart = strstr(cmdseg, "-I");
    if (ptokenstart)
        *(ptokenstart + 1) = 'D';
#endif

    ptokenstart = strstr(cmdseg, "%s");
    if (ptokenstart) {
        *ptokenstart = '-';
        *(ptokenstart + 1) = 'D';
    }
    //bcmSystemEx(cmdseg, 0);
    system(cmdseg);
#ifdef GPL_CODE_VIDEO_GUARANTEE
    char cmd[1024];
    sprintf(cmd, "dqos delMAC %s/FF:FF:FF:FF:FF:FF", mac);
    printf("stb_rule cmd=%s", cmd);
    system(cmd);
#endif

    return 0;
}

void AEI_bcmQoscmdExecuteMac(struct optioncmd *optcmd, char *mac)
{
    char *ptokenstart;
    char cmdseg[256];
    char tempstr[256] = { 0 };
    char *command = optcmd->command;

    printf("AEI_bcmQoscmdExecuteMac, command:%s\r\n", command);

    AEI_bcmQoscmdDelMac(optcmd, mac);
    strcpy(cmdseg, command);
    ptokenstart = strstr(cmdseg, "--ip-src");
    strcpy(ptokenstart, "--src ");
    strcat(ptokenstart, mac);
    //strcat(ptokenstart, " ");
    strcat(ptokenstart, strstr(command, "]") + 1);

    ptokenstart = strstr(cmdseg, "%s");
    if (ptokenstart) {
        *ptokenstart = '-';
        *(ptokenstart + 1) = 'I';
    }
    ptokenstart = strstr(cmdseg, "-p");
    if (ptokenstart) {
        strcpy(tempstr, ptokenstart);
        sprintf(ptokenstart, "1 %s", tempstr);
    }
    //bcmSystemEx(cmdseg, 0);
    printf("final cmd:%s\r\n", cmdseg);
    system(cmdseg);
#ifdef GPL_CODE_VIDEO_GUARANTEE
    char cmd[1024];
    sprintf(cmd, "dqos addMAC %s/FF:FF:FF:FF:FF:FF", mac);
    printf("stb_rule cmd=%s", cmd);
    system(cmd);
#endif
}

//apply QoS rules based on mac address of lan devices (STBs, etc), which request IP with specific option 60 string
int AEI_bcmAddQosMac(struct optioncmd *optcmd, char *mac)
{
    if (optcmd == NULL)
        return -1;
    if (mac == NULL)
        return -1;

//    printf("AEI_bcmAddQosMac, cmd:%s, mac:%s\r\n", optcmd, mac);

    if (strlen(mac) < 12)
        return -1;
    //store the mac address in the option cmd linklist
    if (strlen(optcmd->MACAddresses) > 0) {
        if (strstr(optcmd->MACAddresses, mac))
            return 0;
        sprintf(&optcmd->MACAddresses[strlen(optcmd->MACAddresses)], ";%s", mac);
    } else
        sprintf(&optcmd->MACAddresses[0], "%s", mac);
    AEI_bcmQoscmdExecuteMac(optcmd, mac);
    AEI_send_macs_to_cms(optcmd->clsname, mac, NULL);
    return 1;
}

//delete all ebtables rules related to one option60 string, if the qos rules have been removed
int AEI_bcmQoscmdDelMacs(struct optioncmd *optcmd)
{
    char *macStr = strdup(optcmd->MACAddresses);
    char *tmp = macStr;
    char *pnext = NULL;
    //while loop to delete the ebtables rules by mac address
//      printf("AEI_bcmQoscmdDelMacs, macStr:%s\r\n",macStr);
    while (macStr) {
        pnext = strstr(macStr, ";");
        if (pnext) {
            *pnext = 0;
            pnext++;
        }
        AEI_bcmQoscmdDelMac(optcmd, macStr);
        macStr = pnext;
    }
    //release the mac string memory
    if (tmp)
        free(tmp);

    return 0;
}

//compare two string, ignore the case
int AEI_checkStrMatch(const char *str1, const char *str2)
{
    char temp_str1[128] = { 0 };
    char temp_str2[128] = { 0 };
    char *p;
    unsigned int i;
    if (str1 == NULL || str2 == NULL)
        return 0;
    if (strlen(str1) == 0 || strlen(str2) == 0 || strlen(str1) >= 128 || strlen(str2) >= 128)
        return 0;
    strcpy(temp_str1, str1);
    strcpy(temp_str2, str2);
    p = temp_str1;
    for (i = 0; i < strlen(temp_str1); i++, p++)
        *p = toupper(*p);
    p = temp_str2;
    for (i = 0; i < strlen(temp_str2); i++, p++)
        *p = toupper(*p);
    if (!strcmp(temp_str1, temp_str2))
        return 1;               //require the option60 is the same
    return 0;
}

char macaddressStr[20] = { 0 };

void AEI_bcmSearchOptCmdByVendorId(struct dhcpMessage *oldpacket, const char *vendorid)
{
    struct optioncmd *pnode = optioncmdHead;
    char vendorString[256] = { 0 };
    char tempcmd[256] = { 0 };
    char *p = NULL, *pnext = NULL;

//   printf("AEI_bcmSearchOptCmdByVendorId, vendorid:%s\r\n",vendorid);

    if (vendorid) {
        int len = vendorid[-1];
        if (len >= (int)sizeof(vendorString))
            len = sizeof(vendorString) - 1;
        snprintf(vendorString, len + 1, "%s", vendorid);
    } else {
        return;
    }

    //search on the option linklist via the vendor string
    for (; pnode != NULL; pnode = pnode->pnext) {
        if (pnode->action == 'A' || pnode->action == 'I') {
            if (pnode->optionnum != 0) {
//            printf("optionnum=%d\r\n", pnode->optionnum);
//            printf("command %s\r\n", pnode->command);

                //extract the vendor string from the command
                sprintf(tempcmd, "%s", pnode->command);
                p = strstr(tempcmd, "[");
                if (p) {
                    p++;
                    pnext = strstr(p, "]");
                    if (pnext)
                        *pnext = 0;
                    else
                        continue;
                } else
                    continue;

                //if we find the cmd that match the vendor string
//            if(AEI_checkStrMatch(vendorString,p)==1){
//            use AEI_wstrcmp to support wildcard in the vendor string
//            printf("AEI_bcmSearchOptCmdByVendorId, vendorString:%s, curStr:%s\r\n",vendorString,p);
                if (!AEI_wstrcmp(p, vendorString)) {
                    sprintf(macaddressStr, "%02X:%02X:%02X:%02X:%02X:%02X", oldpacket->chaddr[0],
                            oldpacket->chaddr[1],
                            oldpacket->chaddr[2], oldpacket->chaddr[3], oldpacket->chaddr[4], oldpacket->chaddr[5]);
                    //      printf("found the vendor string:%s, macaddress:%s\r\n", vendorString, macaddressStr);
                    //apply qos rule based on the mac
                    AEI_bcmAddQosMac(pnode, &macaddressStr[0]);
                }
            }
        }
    }

    /* cover case where after factory restore default, STB gets IP before WAN link comes up so
       need to store MAC address in config/datamodel so pass vendor id and mac and let ssk decide
     */
    if (optioncmdHead == NULL) {
#if defined(GPL_CODE)
        /* if can do checking here, can cut down messaging back to ssk which will check again */
        if (!is_stb(vendorString))
            return;
#endif
        sprintf(macaddressStr, "%02X:%02X:%02X:%02X:%02X:%02X", oldpacket->chaddr[0],
                oldpacket->chaddr[1],
                oldpacket->chaddr[2], oldpacket->chaddr[3], oldpacket->chaddr[4], oldpacket->chaddr[5]);
        AEI_send_macs_to_cms(NULL, macaddressStr, vendorString);
    }
}
#endif /* AEI_VDSL_QOS */

char bcmParseCmdAction(char *cmd)
{
   char *token;
   char action = '\0';

   if ((token = strstr(cmd, "-A ")) == NULL)
   {
      if ((token = strstr(cmd, "-I ")) == NULL)
      {
         token = strstr(cmd, "-D ");
      }
   }
   if (token != NULL)
   {
      action = token[1];

      /* replace the command token with %s */
      token[0] = '%';
      token[1] = 's';
   }

   return action;

}  /* End of bcmParseCmdAction() */

void bcmSetQosRule(char action, char *command, u_int32_t leaseip)
{
   char *ptokenstart;
   char cmdseg[1024];
   char actionStr[3];   /* -A or -I or -D */
   struct in_addr ip;
#if defined(GPL_CODE)
   char tempstr[256] = {0};
#endif

#if defined(GPL_CODE_COVERITY_FIX)
    /*CID 12255,Copy into fixed size buffer*/
    strlcpy(cmdseg, command, sizeof(cmdseg));
    ptokenstart = strstr(cmdseg, "[");
    /*CID 11319 Dereference null return value*/
    if (ptokenstart != NULL){
        ip.s_addr   = leaseip;
        strcpy(ptokenstart, inet_ntoa(ip));
    }
    else {
        printf("bcmSetQosRule:No [ found in cmdseg\r\n");
    }
    if(strstr(command,"]") != NULL) {
        /*CID 12255, Copy into fixed size buffer*/
        strlcat(cmdseg, strstr(command, "]") + 1, sizeof(cmdseg));
    }
    else {
        printf("bcmSetQosRule:No ] found in cmdseg\r\n");
    }
#else
   strcpy(cmdseg, command);
   ptokenstart = strstr(cmdseg, "[");
   ip.s_addr   = leaseip;
   strcpy(ptokenstart, inet_ntoa(ip));
   strcat(cmdseg, strstr(command, "]") + 1);
#endif
#if defined(GPL_CODE)
   if (action == 'A')
   {
       // we should insert this rule, not append.
       sprintf(actionStr, "-%c", 'I');

       ptokenstart = strstr(cmdseg, "-p");
       if (ptokenstart) {
           strcpy(tempstr, ptokenstart);
           sprintf(ptokenstart, "1 %s", tempstr);
       }
   }
   else
       sprintf(actionStr, "-%c", action);
#else
   sprintf(actionStr, "-%c", action);
#endif
   sprintf(cmdseg, cmdseg, actionStr);
   system(cmdseg);
    
}  /* End of bcmSetQosRule() */

void bcmAddOptCmdIP(struct optioncmd * optcmd, u_int32_t leaseip, int index)
{
   /* if lease ip address is the same and the QoS rule has been executed, do nothing */  
   if (optcmd->execip[index].ipaddr != leaseip || !optcmd->execip[index].execflag)
   {
      if (optcmd->execip[index].execflag)
      {
         /* delete the QoS rule with the old lease ip */
         bcmSetQosRule('D', optcmd->command, optcmd->execip[index].ipaddr);
         optcmd->execip[index].execflag = 0;
      }
      optcmd->execip[index].ipaddr = leaseip;
      optcmd->execip[index].execflag = 1;

      /* add QoS rule with the new lease ip */
#if defined(GPL_CODE)
      // first, we must delete old rule, in case duplicate rule
      bcmSetQosRule('D', optcmd->command, leaseip);
#endif
      bcmSetQosRule(optcmd->action, optcmd->command, leaseip);
   }
}  /* End of bcmAddOptCmdIP() */

void bcmExecOptCmd(void)
{
   struct optioncmd *pnode;
	struct iface_config_t *iface;
	uint32_t i;

   /* execute all the commands in the option command list */
   for (pnode = optioncmdHead; pnode != NULL; pnode = pnode->pnext)
   {
	   for (iface = iface_config; iface; iface = iface->next)
	   {
		   for (i = 0; i < iface->max_leases; i++)
		   {
            /* skip if lease expires */
            if (lease_expired(&(iface->leases[i])))
               continue;

			   switch (pnode->optionnum)
			   {
				   case DHCP_VENDOR:
#if !defined(GPL_CODE_QOS)
                    //For TELUS project, we will apply ebtables rules using the mac address of the LAN device, rather than IP address,
                    //which is more accurate
					   if (!strcmp(iface->leases[i].vendorid,	pnode->optionval))
					   {
						   bcmAddOptCmdIP(pnode, iface->leases[i].yiaddr, i);
					   }
#endif
					   break;
				   case DHCP_CLIENT_ID:
					   //printf("op61 not implement, please use the MAC filter\r\n");
					   break;
				   case DHCP_USER_CLASS_ID:
					   if (!strcmp(iface->leases[i].classid, pnode->optionval))
					   {
						   bcmAddOptCmdIP(pnode, iface->leases[i].yiaddr, i);
					   }
					   break;
				   default:
					   break;
			   }	
		   }
	   }
   }
}  /* End of bcmExecOptCmd() */

struct optioncmd * bcmAddOptCmd(int optionnum, char action, char *cmd)
{
   struct optioncmd *p, *pnode;
   char *ptokenstart, *ptokenend;

#if defined(GPL_CODE_QOS)
    char classname[256];
    char *token;
    unsigned int len;

    //for mac based option60 qos, we need to extract clsname from the message
    token = strchr(cmd, '|');
    if (token) {
        len = token - cmd;
        if (len <= 0)
            return NULL;
        strncpy(classname, cmd, sizeof(classname));
        
        if(len >= sizeof(classname))
            classname[255] = '\0';
        else
            classname[len] = '\0';
        //get the ebtable cmd
        cmd = token + 1;
        //      printf("bcmAddOptCmd, classname:%s, cmd:%s\r\n", classname, cmd);
    } else {
        printf("wrong format message\r\n");
        return NULL;
    }
#endif

   for (pnode = optioncmdHead; pnode != NULL; pnode = pnode->pnext)
   {
      if (!strcmp(pnode->command, cmd))
         return NULL;
   }	

   pnode = (struct optioncmd *)malloc(sizeof(struct optioncmd));
   if ( pnode == NULL )
   {
      cmsLog_error("malloc failed");
      return NULL;
   }

   memset(pnode, 0, sizeof(struct optioncmd));	
#if defined(GPL_CODE_COVERITY_FIX)
    /*CID12254, Copy into fixed size buffer*/
    strlcpy(pnode->command, cmd, sizeof(pnode->command));
#else
   strcpy(pnode->command, cmd);
#endif
   pnode->action = action;
   pnode->optionnum = optionnum;
#if defined(GPL_CODE_QOS)
    strcpy(pnode->clsname, classname);
#endif
   ptokenstart = strstr(cmd, "[");
   ptokenend = strstr(cmd, "]");
   strncpy(pnode->optionval, ptokenstart + 1, (size_t)(ptokenend - ptokenstart - 1));
   pnode->optionval[ptokenend - ptokenstart - 1] = '\0';
   p = optioncmdHead;	
   optioncmdHead = pnode;
   optioncmdHead->pnext = p;
   return pnode;

}  /* End of bcmAddOptCmd() */

void bcmDelOptCmd(char *cmd)
{
   struct optioncmd *pnode, *pprevnode;
   int i;

#if defined(GPL_CODE_QOS)
    char *token;

    token = strchr(cmd, '|');
    if (token) {
        //get the ebtable cmd
        cmd = token + 1;
//              printf("bcmDelOptCmd, cmd:%s\r\n", cmd);
    } else {
        printf("wrong format message\r\n");
        return ;
    }
#endif

   pnode = pprevnode = optioncmdHead;
   for ( ; pnode != NULL;)
   {
      if (!strcmp(pnode->command, cmd))
      {
         /* delete all the ebtables or iptables rules that had been executed */
         for (i = 0; i < 254; i++)
         {
            if (pnode->execip[i].execflag)
            {
               bcmSetQosRule('D', pnode->command, pnode->execip[i].ipaddr);
               pnode->execip[i].execflag = 0;
            }
         }

#if defined(GPL_CODE_QOS)
            // we need to delete all the ebtables rules related to this vender ID
//      printf("delete option cmd mac:%s\r\n",cmd);
            AEI_bcmQoscmdDelMacs(pnode);
#endif

         /* delete the option command node from the list */	       
         if (optioncmdHead == pnode)
            optioncmdHead = pnode->pnext;
         else
            pprevnode->pnext = pnode->pnext;
         free(pnode);
         break;
      }
      else
      {
         pprevnode = pnode;
         pnode = pnode->pnext;
      }
   }
}  /* End of bcmDelOptCmd() */

void bcmQosDhcp(int optionnum, char *cmd)
{
   char action;

	action = bcmParseCmdAction(cmd);

	switch (action)
	{
		case 'A':
		case 'I':
			if (bcmAddOptCmd(optionnum, action, cmd) != NULL)
            bcmExecOptCmd();
         else
            cmsLog_error("bcmAddOptCmd returns error");
			break;
		case 'D':
			bcmDelOptCmd(cmd);
			break;
		default:
			cmsLog_error("incorrect command action");
			break;
	}
}  /* End of bcmQosDhcp() */

void bcmDelObsoleteRules(void)
{
   struct optioncmd *pnode;
	struct iface_config_t *iface;
   uint32_t delete;
   uint32_t i;

   for (pnode = optioncmdHead; pnode != NULL; pnode = pnode->pnext)
   {
      delete = 1;      
	   for (iface = iface_config; iface && delete; iface = iface->next)
	   {
		   for (i = 0; (i < iface->max_leases) && delete; i++)
		   {
            if (lease_expired(&(iface->leases[i])))
               continue;

			   switch (pnode->optionnum)
			   {
				   case DHCP_VENDOR:
					   if (!strcmp(iface->leases[i].vendorid,	pnode->optionval))
                     delete = 0;
					   break;
				   case DHCP_CLIENT_ID:
					   //printf("op61 not implement, please use the MAC filter\r\n");
					   break;
				   case DHCP_USER_CLASS_ID:
					   if (!strcmp(iface->leases[i].classid, pnode->optionval))
                     delete = 0;
					   break;
				   default:
					   break;
			   }	
		   }
	   }

      if (delete)
      {
         /* delete all the ebtables or iptables rules that had been executed */
         for (i = 0; i < 254; i++)
         {
            if (pnode->execip[i].execflag)
            {
               bcmSetQosRule('D', pnode->command, pnode->execip[i].ipaddr);
               pnode->execip[i].execflag = 0;
            }
         }
      }
   }
}  /* End of bcmDelObsoleteRules() */

