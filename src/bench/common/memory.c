#include <support.h>
#include <common.h>

#ifdef CONTEXT_SW
//#define STANDARD_MALLOC
#endif

#ifdef STANDARD_MALLOC
#include <malloc.h>

#define HEAP_AVAILABLE 60000000

static int sp_allocated = 0;

void sp_init_mem_pool()   { }
void sp_init_mem_single() { }

void *sp_malloc(size_t size)
{
  void* ret;
  nf_lock(MALLOCFREE_LOCK);
  sp_allocated += size;
  if(sp_allocated >= HEAP_AVAILABLE)
    {
      log("ALLOCATION FAILED, MEMORY FILLED\n");
      sp_allocated -= size;
      nf_unlock(MALLOCFREE_LOCK);
      return NULL;
    }
  ret = malloc(size);
  log("total allocated size is %d, last increment %d addr %#x\n", 
      sp_allocated, size, (int)ret);
  //log("%#x\n", (int)ret);
  nf_unlock(MALLOCFREE_LOCK);
  return ret;
}


#define PREV_INUSE 0x1UL
struct mem_struct
{
  int prev_size; /* Size of previous chunk (if free). */
  int size;      /* Size in bytes, including overhead. */
};


void sp_free(void *ptr)
{
  nf_lock(MALLOCFREE_LOCK);
  struct mem_struct* p;   
  p = (struct mem_struct*)((char*)(ptr)-8);
  int size = p->size & ~PREV_INUSE;
  size -= 4;
  sp_allocated -= size;
  log("total allocated size is %d, last decrement %d addr %#x\n", 
      sp_allocated, size, (int)ptr);
  
  free(ptr);
  nf_unlock(MALLOCFREE_LOCK);
}

void* sp_realloc(void *ptr, int new_size)
{
  void* ret;
  nf_lock(MALLOCFREE_LOCK);
  struct mem_struct* p;   
  p = (struct mem_struct*)((char*)(ptr)-8);
  int size = p->size & ~PREV_INUSE;
  size -= 4;
  sp_allocated = sp_allocated - size + new_size;
  if(sp_allocated >= HEAP_AVAILABLE)
    {
      log("ALLOCATION FAILED, MEMORY FILLED\n");
      sp_allocated -= size;
      nf_unlock(MALLOCFREE_LOCK);
      return NULL;
    }

  ret = realloc(ptr,new_size);
  nf_unlock(MALLOCFREE_LOCK);
  return ret;
}

#else    /*------------------------------*/

#include <num_cpu.h>
extern char _end[]; 
extern char _sp[]; 

void *anl_malloc (unsigned size, void** __mp__);
void anl_free (void *memp, void** __mp__);
void *anl_realloc (void *oldp, unsigned size, void** __mp__);
void anl_init_mempool (void *pool, unsigned size);


char* sp_mem_pool[MAX_THREAD];
int   sp_mem_allocated[MAX_THREAD];

#define ALLOCATOR_TRACK_AVAILABLE 1

int HEAP_AVAILABLE;
#define BASE_MASK 0x4000000
#define NOMANSLAND 0x104

void sp_init_mem_single()
{
#ifdef CONTEXT_SW
  HEAP_AVAILABLE = 64*1024*1024;
  sp_mem_pool[0] = (char*)malloc(HEAP_AVAILABLE);
#else
  HEAP_AVAILABLE = (((int)_sp - BASE_MASK)   // top of stack/memory
		    - (((int)_end) - BASE_MASK)   // static data 
		    - (STACK_IN_BYTES*MAX_THREAD) - STACK_SKIP - DCACHE // stack space
		    - NOMANSLAND); // for sim
  sp_mem_pool[0] = &(_end[4]);
#endif

  HEAP_AVAILABLE = HEAP_AVAILABLE & (~3);

  log("total mem: %d bytes of dynamic allocation, base %x\n",
      HEAP_AVAILABLE, (unsigned)sp_mem_pool[0]);

  sp_mem_allocated[0] = 0;
  anl_init_mempool(sp_mem_pool[0], HEAP_AVAILABLE);
}

struct __mp__  {                      /* memory pool */
  struct __mp__     *next;        /* single-linked list */
  unsigned              len;        /* length of following block */
};

void sp_init_mem_pool()
{
  int i;
  log("heap remaining out of %d: ", HEAP_AVAILABLE);
  HEAP_AVAILABLE -= sp_mem_allocated[0];
  log("%d\n", HEAP_AVAILABLE);

  HEAP_AVAILABLE = HEAP_AVAILABLE / MAX_THREAD;
  HEAP_AVAILABLE = HEAP_AVAILABLE & (~3);
  HEAP_AVAILABLE -= 4*sizeof(struct __mp__);  //MIN_BLOCK in MALLOC.c

  // the allocated memory was at the top of the memory, use the bottom
  // this assumes that there was no free() that made the allocation go lower
  // than the allocated memory mark
  log("each thread is allowed %d bytes of dynamic allocation\n", HEAP_AVAILABLE);

  for(i=0; i<MAX_THREAD; i++)
    {
      if(i==0)
	{
	  ; //sp_mem_pool[i] = &(_end[4]);
	}
      else
	sp_mem_pool[i] = sp_mem_pool[i-1] + HEAP_AVAILABLE;

      log("tid %d, base %x\n", i, (unsigned)sp_mem_pool[i]);
      sp_mem_allocated[i] = 0;
      anl_init_mempool(sp_mem_pool[i], HEAP_AVAILABLE);
    }
}

void *sp_malloc(size_t size)
{
  void* ret;
  int tid = nf_tid();

  if((size & 3) != 0)  // make it a multiple of 32 bits to avoid unaligned accesses
    size = (size & (~3)) + 4;

#if ALLOCATOR_TRACK_AVAILABLE
  sp_mem_allocated[tid] = sp_mem_allocated[tid] + size + sizeof(struct __mp__);

  if(sp_mem_allocated[tid] >= HEAP_AVAILABLE)
    {
        log("tid %d, total allocated size is %d, last increment %d \n"
            "tid %d, ALLOCATION FAILED, MEMORY FILLED\n",
            tid, sp_mem_allocated[tid], size, tid);
      sp_mem_allocated[tid] -= size;
      return NULL;
    }
#endif
  ret = anl_malloc(size, (void**)&sp_mem_pool[tid]);
  
  log("tid %d, total allocated size is %d, last increment %d addr %#x\n", 
      tid, sp_mem_allocated[tid], size, (int)ret);  
  //log("%#x\n", (int)ret);
  return ret;
}

void sp_free(void *ptr)
{
  int tid = nf_tid();
  int sz = (((struct __mp__  *)ptr) [-1]).len;

#if ALLOCATOR_TRACK_AVAILABLE
  sp_mem_allocated[tid] = sp_mem_allocated[tid]  - (sz + sizeof(struct __mp__));
#endif
  log("tid %d, total allocated size is %d, last decrement %d addr %#x\n", 
      tid, sp_mem_allocated[tid], sz, (int)ptr);
  
  anl_free(ptr, (void**)(&sp_mem_pool[tid]));
}

void* sp_realloc(void *ptr, int new_size)
{
  void* ret;
  int tid = nf_tid();

  if((new_size & 3) != 0)  // make it a multiple of 32 bits to avoid unaligned accesses
    new_size = (new_size & (~3)) + 4;

  int sz = (((struct __mp__  *)ptr) [-1]).len;

#if ALLOCATOR_TRACK_AVAILABLE
  sp_mem_allocated[tid] = sp_mem_allocated[tid] + new_size - sz;
  if(sp_mem_allocated[tid] >= HEAP_AVAILABLE)
    {
      log("ALLOCATION FAILED, MEMORY FILLED\n");
      sp_mem_allocated[tid] -= sz;
      return NULL;
    }
#endif

  ret = anl_realloc(ptr,new_size, (void**) &sp_mem_pool[tid]);
  return ret;
}

#endif
