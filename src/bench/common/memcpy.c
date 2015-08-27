#include "support.h"

//volatile int mydetect = 0;

// Copies words when possible. This should be faster if your
// src and dst addresses are equal mod 4.
void *memcpy32(void *dst, const void *src, size_t count) {

/*  if((count < 20) || (count >= 1600) || mydetect)
    {
      mydetect = 1;
      nf_lock(2);
      nf_lock(2); // will not return
    }*/

  // too small
  if (count < 10) {
    return memcpy(dst, src, count);
  }

  int offset = (int)dst & 3;  
  if (offset != ((int)src & 3)) {
    // src and dst don't line up
    log("t%d: WARNING: misaligned copy\n", nf_tid());
    return memcpy(dst, src, count);
  }

  char *cdst = (char*)dst;
  char *csrc = (char*)src;
  // copy unaligned 0-3 bytes at front 
  if (offset != 0) {
    offset = 4 - offset;
    switch (offset) {
    case 3:
      cdst[2] = csrc[2];
    case 2:
      cdst[1] = csrc[1];
    case 1:
      cdst[0] = csrc[0];
      
      // round up to the next word aligned src and dst
      cdst += offset;
      csrc += offset;
      // round down the count
      count -= offset;
    }
  }

  // copy unaligned 0-3 bytes at end
  switch (count & 3) {
  case 3:
    cdst[count - 3] = csrc[count - 3];
  case 2:
    cdst[count - 2] = csrc[count - 2];
  case 1:
    cdst[count - 1] = csrc[count - 1];
  }

  // get read to copy ints
  int *dst32 = (int *)cdst;
  int *src32= (int *)csrc;
  count >>= 2;
  int *end = src32 + count;
  
  // align on a 16 byte boundry
  switch (count & 3) {
  case 3:
    dst32[2] = src32[2];
  case 2:
    dst32[1] = src32[1];
  case 1:
    dst32[0] = src32[0];
    
    dst32 += (count & 3);
    src32 += (count & 3);
  }

  // do the main work of copying 16 bytes 
  // at a time
  while (src32 != end) {
    dst32[0] = src32[0];
    dst32[1] = src32[1];
    dst32[2] = src32[2];
    dst32[3] = src32[3];
    src32+=4;
    dst32+=4;
  }

  return dst;
}

