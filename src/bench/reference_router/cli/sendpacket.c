#include <fcntl.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <sys/uio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <assert.h>
#include <sys/select.h>
#include <unistd.h>


int global_handle = -1;

static int get_iface_index(int fd, const int8_t *device) {
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device, sizeof(ifr.ifr_name));

    if (ioctl(fd, SIOCGIFINDEX, &ifr) == -1) 
      {
	fprintf(stdout, "ioctl: %s", strerror(errno));
	exit (-1);
      }
    
    return ifr.ifr_ifindex;
}              

void sendpacket_open_pf(const char *device)
{
    int mysocket;
    struct ifreq ifr;
    struct sockaddr_ll sa;
    int n = 1, err;
    socklen_t errlen = sizeof(err);

    assert(device);
   
    fprintf(stdout, "sendpacket: using PF_PACKET\n");

    /* open our socket */
    if ((mysocket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
      fprintf(stdout, "socket: %s", strerror(errno));
      exit(-1);
    }

   
    /* get the interface id for the device */
    if ((sa.sll_ifindex = get_iface_index(mysocket, device)) < 0) {
        close(mysocket);
        exit(-1);
    }

    /* bind socket to our interface id */
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_ALL);
    if (bind(mysocket, (struct sockaddr *)&sa, sizeof(sa)) < 0) 
      {
	fprintf(stdout, "bind error: %s", strerror(errno));
	close(mysocket);
        exit(-1);
      }
    
    /* check for errors, network down, etc... */
    if (getsockopt(mysocket, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0) 
      {
	fprintf(stdout, "error opening %s: %s", device, strerror(errno));
        close(mysocket);
        exit(-1);
      }
    
    if (err > 0) 
      {
        fprintf(stdout, "error opening %s: %s", device,  strerror(err));
        close(mysocket);
	exit(-1);
    }
    
    global_handle = mysocket;
}



int sendpacket(const u_char *data, size_t len)
{
    assert(data);
        
   if (len <= 0)
        return -1; 
   int retcode;

   fflush(NULL);

 TRY_SEND_AGAIN:
   retcode = (int)send(global_handle, (void *)data, len, 0);
   
   /* out of buffers, or hit max PHY speed, silently retry */
   if (retcode < 0) {
     switch (errno) {
     case EAGAIN:
       goto TRY_SEND_AGAIN;
       break;
     case ENOBUFS:
       goto TRY_SEND_AGAIN;
       break;
       
     default:
       fprintf(stdout, "Error in sendpacket: %s (%d)",  strerror(errno), errno);
     }
   } 
   return retcode;

}

int get_packet(char* recv_buf, int len)
{
  ssize_t retcode = 0;
  //fprintf(stdout, "in get_packet\n");

  int rc=0;
  struct timeval       timeout;
 fd_set        master_set, working_set;

 TRY_RECV_AGAIN:
  
   /*************************************************************/
   /* Initialize the master fd_set                              */
   /*************************************************************/
   FD_ZERO(&master_set);
   FD_SET(global_handle, &master_set);

   /*************************************************************/
   /* Initialize the timeval struct to 3 minutes.  If no        */
   /* activity after 3 minutes this program will end.           */
   /*************************************************************/
   timeout.tv_sec  = 3 * 60;
   timeout.tv_usec = 0;

   /*************************************************************/
   /* Loop waiting for incoming connects or for incoming data   */
   /* on any of the connected sockets.                          */
   /*************************************************************/
   do
   {
      /**********************************************************/
      /* Copy the master fd_set over to the working fd_set.     */
      /**********************************************************/
      memcpy(&working_set, &master_set, sizeof(master_set));

      /**********************************************************/
      /* Call select() and wait 5 minutes for it to complete.   */
      /**********************************************************/
      rc = select(global_handle + 1, &working_set, NULL, NULL, &timeout);

      /**********************************************************/
      /* Check to see if the select call failed.                */
      /**********************************************************/
      if (rc < 0)
      {
         perror("  select() failed");
         break;
      }

      /**********************************************************/
      /* Check to see if the 5 minute time out expired.         */
      /**********************************************************/
      if(rc == 0)
      {
         printf("  select() timed out.  End program.\n");
      }
   } while (rc == 0);


   if (FD_ISSET(global_handle, &working_set))
     ; //printf("  received packet.\n");
   else
     goto TRY_RECV_AGAIN;

  retcode = recv(global_handle, recv_buf, len, 0);
  
  if(retcode < 0) {
     switch (errno) {
     case EAGAIN:
       goto TRY_RECV_AGAIN;
       break;
     case ENOBUFS:
       goto TRY_RECV_AGAIN;
       break;
       
     default:
       fprintf(stdout, "Error in get_packet: %s (%d)",  strerror(errno), errno);
     }
   } 

  if(retcode != len/3)
    goto TRY_RECV_AGAIN;

  //fprintf(stdout, "got packet of size %d\n", retcode);
  return retcode;
}



/*
 * Please note that some of this code was copied from tcpreplay v3.4.0 with the following license
 */

 /*
 * Copyright (c) 2006 Aaron Turner.
 * Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 * Copyright (c) 2000 Torsten Landschoff <torsten@debian.org>
 *                    Sebastian Krahmer  <krahmer@cs.uni-potsdam.de>
 * Copyright (c) 1993, 1994, 1995, 1996, 1998
 *      The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright owners nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    display the following acknowledgement:
 *    ``This product includes software developed by the University of 
 *    California,  Lawrence Berkeley Laboratory and its contributors.''
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
