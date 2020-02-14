extern "C"
{
    #include "cms_msg.h"
    #include "cms_tmr.h"
    #include "cms_mdm.h"
    #include "cms_eid.h"
}
#include <cassert>
#include <cstring>
#include <iostream>
#include <cstdlib>   

#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#endif

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "udp.h"
#include "stun.h"

using namespace std;

//void *tmrHandle = NULL;
void *msgHandle = NULL;

void
usage()
{
   cerr << "Usage:" << endl
	<< "    ./client  [-v] [-p srcPort]" << endl;
}

void sig_handle_exit(int sig_num)
{
	fprintf(stderr, "******stunc:%s() recv signal 0x%x******\n", __func__, sig_num);
    aei_exit();
	exit(0);
}


#define MAX_NIC 3

int
main(int argc, char* argv[])
{
   assert( sizeof(UInt8 ) == 1 );
   assert( sizeof(UInt16) == 2 );
   assert( sizeof(UInt32) == 4 );
    
   bool verbose = false;
   int srcPort=0;
   StunAddress4 sAddr;
   SINT32  shmId = 0;
   CmsRet  ret;

   
   signal(SIGTERM, &sig_handle_exit);   

   for ( int arg = 1; arg<argc; arg++ )
   {
      if ( !strcmp( argv[arg] , "-v" ) )
      {
         verbose = true;
      }
      else if ( !strcmp( argv[arg] , "-p" ) )
      {
         arg++;
         if ( argc <= arg ) 
         {
            usage();
            exit(-1);
         }
         srcPort = strtol( argv[arg], NULL, 10);
      }else{
         usage();
         exit(-1);
      }

   }

   sAddr.port=srcPort;
   sAddr.addr=0;
   
   if ((ret = cmsMsg_initWithFlags(EID_STUND, 0, &msgHandle)) != CMSRET_SUCCESS) {
      cerr << "[STUN] cmsMsg_initWithFlags error...." << endl;
	  exit(-1);
   }

   aei_stun4Tr69( verbose, srcPort, &sAddr);
 
   return 0;
}


