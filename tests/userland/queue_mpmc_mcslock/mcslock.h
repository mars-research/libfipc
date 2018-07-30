/**
 * @File     : mcslock.h
 * @Author   : Jiwon Jeon
 */

#include <stdint.h>

#ifndef LIBFIPC_TEST
#include "../libfipc_test.h"
#endif

#ifndef __KERNEL__ 
#define pr_err printf 
#endif 

#define MAX_MCS_LOCKS        2

// Types
typedef uint64_t data_t;

typedef struct qnode {
    volatile void* CACHE_ALIGNED next; 
    volatile char CACHE_ALIGNED waiting; 
} qnode; 

/*
typedef struct {
    struct qnode* CACHE_ALIGNED v;
    int CACHE_ALIGNED lock_idx;
} mcslock;
*/
typedef qnode mcslock;

typedef struct node {
	uint64_t CACHE_ALIGNED field;	
} node_t;

/*
static inline uint64_t
cmp_and_swap_atomic(mcslock* L, uint64_t cmpval, uint64_t newval)
{
    uint64_t out;
    __asm__ volatile(
                "lock; cmpxchgq %2, %1"
                //: "=a" (out), "+m" ((L->v)->locked)
                : "=a" (out), "+m" (L->v)
                : "q" (newval), "0"(cmpval)
                : "cc");
    return out == cmpval;
}
*/

static inline qnode*
fetch_and_store ( mcslock* L, qnode* val )
{
    __asm__ volatile(
                "lock; xchgq %0, %1\n\t"
                : "+m" (L->v), "+r" (val)
                : 
                : "memory", "cc");
    return val;
}
 
static inline uint64_t
cmp_and_swap ( mcslock *L, uint64_t cmpval, uint64_t newval )
{
    uint64_t out;
    __asm__ volatile(
                "lock; cmpxchgq %2, %1"
                : "=a" (out), "+m" (L->v)
                : "q" (newval), "0"(cmpval)
                : "cc");
    return out == cmpval;
}

void mcs_init_global( mcslock* L );
void mcs_init_local	( qnode* I );
//void mcs_init	( mcslock *L );
void mcs_lock 	( mcslock *L, qnode* I );
void mcs_unlock ( mcslock *L, qnode* I );




