/*----------------------------------------------------------------------*
<:copyright-broadcom

 Copyright (c) 2005 Broadcom Corporation
 All Rights Reserved
 No portions of this material may be reproduced in any form without the
 written permission of:
          Broadcom Corporation
          16215 Alton Parkway
          Irvine, California 92619
 All information contained in this document is Broadcom Corporation
 company private, proprietary, and trade secret.

:>
 *----------------------------------------------------------------------*/
#ifndef MD5_H
#define MD5_H

typedef struct MD5Context {
  u_int32_t buf[4];
  u_int32_t bits[2];
  u_char in[64];
} MD5Context;

void MD5ToAscii( unsigned char *s /*16bytes */, unsigned char *d /*33bytes*/);
#if defined(GPL_CODE_ENABLE_VIDEO_SSID)
void AEI_getActMd5(unsigned char *inputStr,unsigned char *digest);
#endif

#endif
