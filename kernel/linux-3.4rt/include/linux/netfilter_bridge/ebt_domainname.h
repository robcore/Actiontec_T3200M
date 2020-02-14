#ifndef __LINUX_BRIDGE_EBT_DOMAINNAME_H
#define __LINUX_BRIDGE_EBT_DOMAINNAME_H

#include <linux/types.h>

#define EBT_DOMAINNAME  0x01
#define EBT_DOMAINNAME_MASK (EBT_DOMAINNAME)
#define EBT_SAVEWHITELIST     0x02
#define EBT_SAVEWHITELIST_MASK    (EBT_SAVEWHITELIST)
#define EBT_MATCHWHITELIST    0x04
#define EBT_MATCHWHITELIST_MASK   (EBT_MATCHWHITELIST)

#define EBT_DOMAINNAME_MATCH "domainname"
#define DOMAINNAME_SIZE 255

struct ebt_domainname_info {
      char domainname[DOMAINNAME_SIZE];
      bool saveWhiteList;
      bool matchWhiteList;
};

#endif
