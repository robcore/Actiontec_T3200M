#ifndef __AEI_COMMON_H
#define __AEI_COMMON_H

enum {
   FAC = 0,
   FCC = 1,
};

#define FAC_MASK (1 << FAC)
#define FCC_MASK (1 << FCC)

#define ARRAY_NUM(arr) (sizeof(arr) / sizeof(arr[0]))
#define SENSITIVE_FLAG_PATH "/proc/nvram/sensitive_flag"

typedef struct {
    char **table;
    unsigned int mask;
    char *msg;
} permission_table_t;


#endif
