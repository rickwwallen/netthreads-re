#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include "num_cpu.h"

#ifdef CONTEXT_NF
#define log(f, a...)
/*
#define simprintf(f, a...)
*/
extern void simprintf(char *frmt, ...) 
  __attribute__ ((__format__ (__printf__, 1, 2))); 

#endif

#ifdef CONTEXT_SIM
#define log(f, a...) simprintf(f, ##a)
extern void simprintf(char *frmt, ...)
   __attribute__ ((__format__ (__printf__, 1, 2)));
#endif

#ifdef CONTEXT_SW

#if 1
#define log(f, a...) printf(f, ##a)
#define simprintf(f, a...) printf(f, ##a)
#else
#define log(f, a...) 
#define simprintf(f, a...) 
#endif

#endif

#ifndef _SYS_TYPES_H
typedef unsigned int uint;
#endif

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 * Taken from the linux kernel
 */
#define container_of(ptr, type, member) ({                      \
      const typeof( ((type *)0)->member ) *__mptr = (ptr);      \
      (type *)( (char *)__mptr - offsetof(type,member) );})


#endif
