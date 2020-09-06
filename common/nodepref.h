/**
 * @file memdiff.h
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 * 
 * TBD
 *       
 * @section DESCRIPTION
 *                      
 * This file defines macros to prefetch btree nodes
 */  

#ifndef _BTREE_NODEPREF_H
#define _BTREE_NODEPREF_H

/* --------------------------------------------------------------------- */
/*                      Prefetch Instructions                            */
/* --------------------------------------------------------------------- */
/*
   T0 (temporal data):
       prefetch data into all levels of the cache hierarchy.
       Pentium III processor 1st- or 2nd-level cache.
       Pentium 4 and Intel Xeon processors 2nd-level cache.

   NTA (non-temporal data with respect to all cache levels)
        prefetch data into non-temporal cache structure and
        into a location close to the processor, minimizing cache pollution.
        Pentium III processor 1st-level cache
        Pentium 4 and Intel Xeon processors 2nd-level cache
 */

// We use prefetch instructions by default.
#ifndef NO_PREFETCH

#define prefetcht0(mem_var)     \
        __asm__ __volatile__ ("prefetcht0 %0": :"m"(mem_var))
#define prefetchnta(mem_var)    \
        __asm__ __volatile__ ("prefetchnta %0": :"m"(mem_var))

#define pref(mem_var)      prefetcht0(mem_var)
#define prefnta(mem_var)   prefetchnta(mem_var)

#define ptouch(mem_var) \
        __asm__ __volatile__ ("movl %0, %%eax": :"m"(mem_var):"%eax")

#else // NO_PREFETCH

#define pref(mem_var)
#define prefnta(mem_var)
#define ptouch(mem_var)

#endif 

/* ---------------------------------------------------------------------- */

#ifndef KB
#define KB      (1024)
#endif

#ifndef MB
#define MB      (1024*KB)
#endif

#ifndef GB
#define GB      (1024*MB)
#endif

#define CACHE_LINE_SIZE    64
#define PAGE_SIZE          (4*KB)

// obtain the starting address of a cache line
#define getline(addr) \
(((unsigned long long)(addr)) & (~(unsigned long long)(CACHE_LINE_SIZE-1)))

// check if address is aligned at line boundary
#define isaligned_atline(addr) \
(!(((unsigned long long)(addr)) & (unsigned long long)(CACHE_LINE_SIZE-1)))

/* --------------------------------------------------------------------- */
/*                    Prefetch constant number of lines                  */
/* --------------------------------------------------------------------- */

// e.g. PREF_16(pref, *p, L2_CACHE_LINE);

#define PREF_1(cmd, mem_var)	cmd(mem_var)

#define PREF_2(cmd, mem_var)	\
	PREF_1(cmd, mem_var); cmd(*((char *)&(mem_var) + (CACHE_LINE_SIZE)))

#define PREF_3(cmd, mem_var)	\
	PREF_2(cmd, mem_var); cmd(*((char *)&(mem_var) + 2*(CACHE_LINE_SIZE)))

#define PREF_4(cmd, mem_var)	\
	PREF_3(cmd, mem_var); cmd(*((char *)&(mem_var) + 3*(CACHE_LINE_SIZE)))

#define PREF_5(cmd, mem_var)	\
	PREF_4(cmd, mem_var); cmd(*((char *)&(mem_var) + 4*(CACHE_LINE_SIZE)))

#define PREF_6(cmd, mem_var)	\
	PREF_5(cmd, mem_var); cmd(*((char *)&(mem_var) + 5*(CACHE_LINE_SIZE)))

#define PREF_7(cmd, mem_var)	\
	PREF_6(cmd, mem_var); cmd(*((char *)&(mem_var) + 6*(CACHE_LINE_SIZE)))

#define PREF_8(cmd, mem_var)	\
	PREF_7(cmd, mem_var); cmd(*((char *)&(mem_var) + 7*(CACHE_LINE_SIZE)))

#define PREF_9(cmd, mem_var)	\
	PREF_8(cmd, mem_var); cmd(*((char *)&(mem_var) + 8*(CACHE_LINE_SIZE)))

#define PREF_10(cmd, mem_var)	\
	PREF_9(cmd, mem_var); cmd(*((char *)&(mem_var) + 9*(CACHE_LINE_SIZE)))

#define PREF_11(cmd, mem_var)	\
	PREF_10(cmd, mem_var); cmd(*((char *)&(mem_var) + 10*(CACHE_LINE_SIZE)))

#define PREF_12(cmd, mem_var)	\
	PREF_11(cmd, mem_var); cmd(*((char *)&(mem_var) + 11*(CACHE_LINE_SIZE)))

#define PREF_13(cmd, mem_var)	\
	PREF_12(cmd, mem_var); cmd(*((char *)&(mem_var) + 12*(CACHE_LINE_SIZE)))

#define PREF_14(cmd, mem_var)	\
	PREF_13(cmd, mem_var); cmd(*((char *)&(mem_var) + 13*(CACHE_LINE_SIZE)))

#define PREF_15(cmd, mem_var)	\
	PREF_14(cmd, mem_var); cmd(*((char *)&(mem_var) + 14*(CACHE_LINE_SIZE)))

#define PREF_16(cmd, mem_var)	\
	PREF_15(cmd, mem_var); cmd(*((char *)&(mem_var) + 15*(CACHE_LINE_SIZE)))

#define PREF_17(cmd, mem_var)	\
	PREF_16(cmd, mem_var); cmd(*((char *)&(mem_var) + 16*(CACHE_LINE_SIZE)))

#define PREF_18(cmd, mem_var)	\
	PREF_17(cmd, mem_var); cmd(*((char *)&(mem_var) + 17*(CACHE_LINE_SIZE)))

#define PREF_19(cmd, mem_var)	\
	PREF_18(cmd, mem_var); cmd(*((char *)&(mem_var) + 18*(CACHE_LINE_SIZE)))

#define PREF_20(cmd, mem_var)	\
	PREF_19(cmd, mem_var); cmd(*((char *)&(mem_var) + 19*(CACHE_LINE_SIZE)))

#define PREF_21(cmd, mem_var)	\
	PREF_20(cmd, mem_var); cmd(*((char *)&(mem_var) + 20*(CACHE_LINE_SIZE)))

#define PREF_22(cmd, mem_var)	\
	PREF_21(cmd, mem_var); cmd(*((char *)&(mem_var) + 21*(CACHE_LINE_SIZE)))

#define PREF_23(cmd, mem_var)	\
	PREF_22(cmd, mem_var); cmd(*((char *)&(mem_var) + 22*(CACHE_LINE_SIZE)))

#define PREF_24(cmd, mem_var)	\
	PREF_23(cmd, mem_var); cmd(*((char *)&(mem_var) + 23*(CACHE_LINE_SIZE)))

#define PREF_25(cmd, mem_var)	\
	PREF_24(cmd, mem_var); cmd(*((char *)&(mem_var) + 24*(CACHE_LINE_SIZE)))

#define PREF_26(cmd, mem_var)	\
	PREF_25(cmd, mem_var); cmd(*((char *)&(mem_var) + 25*(CACHE_LINE_SIZE)))

#define PREF_27(cmd, mem_var)	\
	PREF_26(cmd, mem_var); cmd(*((char *)&(mem_var) + 26*(CACHE_LINE_SIZE)))

#define PREF_28(cmd, mem_var)	\
	PREF_27(cmd, mem_var); cmd(*((char *)&(mem_var) + 27*(CACHE_LINE_SIZE)))

#define PREF_29(cmd, mem_var)	\
	PREF_28(cmd, mem_var); cmd(*((char *)&(mem_var) + 28*(CACHE_LINE_SIZE)))

#define PREF_30(cmd, mem_var)	\
	PREF_29(cmd, mem_var); cmd(*((char *)&(mem_var) + 29*(CACHE_LINE_SIZE)))

#define PREF_31(cmd, mem_var)	\
	PREF_30(cmd, mem_var); cmd(*((char *)&(mem_var) + 30*(CACHE_LINE_SIZE)))

#define PREF_32(cmd, mem_var)	\
	PREF_31(cmd, mem_var); cmd(*((char *)&(mem_var) + 31*(CACHE_LINE_SIZE)))

/* ---------------------------------------------------------------------- */
#ifndef NO_PREFETCH

#define LOOP_PREF(cmd, mem_varp, nline)            \
{char *_p= (char *)(mem_varp);                     \
 char *_pend= _p + (nline)*CACHE_LINE_SIZE;        \
 for (; _p<_pend; _p+=(CACHE_LINE_SIZE)) cmd(*_p); \
}

#else // NO_PREFETCH

#define LOOP_PREF(cmd, mem_varp, nline)

#endif

/* ---------------------------------------------------------------------- */

// for non leaf node
static void inline NODE_PREF(register void *bbp)
{
     pref (* ((char *)bbp));
#    if NONLEAF_LINE_NUM >= 2
     pref (* ((char *)bbp + CACHE_LINE_SIZE));
#    endif
#    if NONLEAF_LINE_NUM >= 3
     pref (* ((char *)bbp + CACHE_LINE_SIZE*2));
#    endif
#    if NONLEAF_LINE_NUM >= 4
     pref (* ((char *)bbp + CACHE_LINE_SIZE*3));
#    endif
#    if NONLEAF_LINE_NUM >= 5
     pref (* ((char *)bbp + CACHE_LINE_SIZE*4));
#    endif
#    if NONLEAF_LINE_NUM >= 6
     pref (* ((char *)bbp + CACHE_LINE_SIZE*5));
#    endif
#    if NONLEAF_LINE_NUM >= 7
     pref (* ((char *)bbp + CACHE_LINE_SIZE*6));
#    endif
#    if NONLEAF_LINE_NUM >= 8
     pref (* ((char *)bbp + CACHE_LINE_SIZE*7));
#    endif
#    if NONLEAF_LINE_NUM >= 9
     pref (* ((char *)bbp + CACHE_LINE_SIZE*8));
#    endif
#    if NONLEAF_LINE_NUM >= 10
     pref (* ((char *)bbp + CACHE_LINE_SIZE*9));
#    endif
#    if NONLEAF_LINE_NUM >= 11
     pref (* ((char *)bbp + CACHE_LINE_SIZE*10));
#    endif
#    if NONLEAF_LINE_NUM >= 12
     pref (* ((char *)bbp + CACHE_LINE_SIZE*11));
#    endif
#    if NONLEAF_LINE_NUM >= 13
     pref (* ((char *)bbp + CACHE_LINE_SIZE*12));
#    endif
#    if NONLEAF_LINE_NUM >= 14
     pref (* ((char *)bbp + CACHE_LINE_SIZE*13));
#    endif
#    if NONLEAF_LINE_NUM >= 15
     pref (* ((char *)bbp + CACHE_LINE_SIZE*14));
#    endif
#    if NONLEAF_LINE_NUM >= 16
     pref (* ((char *)bbp + CACHE_LINE_SIZE*15));
#    endif
#    if NONLEAF_LINE_NUM >= 17
     pref (* ((char *)bbp + CACHE_LINE_SIZE*16));
#    endif
#    if NONLEAF_LINE_NUM >= 18
     pref (* ((char *)bbp + CACHE_LINE_SIZE*17));
#    endif
#    if NONLEAF_LINE_NUM >= 19
     pref (* ((char *)bbp + CACHE_LINE_SIZE*18));
#    endif
#    if NONLEAF_LINE_NUM >= 20
     pref (* ((char *)bbp + CACHE_LINE_SIZE*19));
#    endif
#    if NONLEAF_LINE_NUM >= 21
     pref (* ((char *)bbp + CACHE_LINE_SIZE*20));
#    endif
#    if NONLEAF_LINE_NUM >= 22
     pref (* ((char *)bbp + CACHE_LINE_SIZE*21));
#    endif
#    if NONLEAF_LINE_NUM >= 23
     pref (* ((char *)bbp + CACHE_LINE_SIZE*22));
#    endif
#    if NONLEAF_LINE_NUM >= 24
     pref (* ((char *)bbp + CACHE_LINE_SIZE*23));
#    endif
#    if NONLEAF_LINE_NUM >= 25
     pref (* ((char *)bbp + CACHE_LINE_SIZE*24));
#    endif
#    if NONLEAF_LINE_NUM >= 26
     pref (* ((char *)bbp + CACHE_LINE_SIZE*25));
#    endif
#    if NONLEAF_LINE_NUM >= 27
     pref (* ((char *)bbp + CACHE_LINE_SIZE*26));
#    endif
#    if NONLEAF_LINE_NUM >= 28
     pref (* ((char *)bbp + CACHE_LINE_SIZE*27));
#    endif
#    if NONLEAF_LINE_NUM >= 29
     pref (* ((char *)bbp + CACHE_LINE_SIZE*28));
#    endif
#    if NONLEAF_LINE_NUM >= 30
     pref (* ((char *)bbp + CACHE_LINE_SIZE*29));
#    endif
#    if NONLEAF_LINE_NUM >= 31
     pref (* ((char *)bbp + CACHE_LINE_SIZE*30));
#    endif
#    if NONLEAF_LINE_NUM >= 32
     pref (* ((char *)bbp + CACHE_LINE_SIZE*31));
#    endif
#    if NONLEAF_LINE_NUM >= 33
#    error "NONLEAF_LINE_NUM must be <= 32!"
#    endif
}

static void inline LEAF_PREF(register void *bbp)
{
     pref (* ((char *)bbp));
#    if LEAF_LINE_NUM >= 2
     pref (* ((char *)bbp + CACHE_LINE_SIZE));
#    endif
#    if LEAF_LINE_NUM >= 3
     pref (* ((char *)bbp + CACHE_LINE_SIZE*2));
#    endif
#    if LEAF_LINE_NUM >= 4
     pref (* ((char *)bbp + CACHE_LINE_SIZE*3));
#    endif
#    if LEAF_LINE_NUM >= 5
     pref (* ((char *)bbp + CACHE_LINE_SIZE*4));
#    endif
#    if LEAF_LINE_NUM >= 6
     pref (* ((char *)bbp + CACHE_LINE_SIZE*5));
#    endif
#    if LEAF_LINE_NUM >= 7
     pref (* ((char *)bbp + CACHE_LINE_SIZE*6));
#    endif
#    if LEAF_LINE_NUM >= 8
     pref (* ((char *)bbp + CACHE_LINE_SIZE*7));
#    endif
#    if LEAF_LINE_NUM >= 9
     pref (* ((char *)bbp + CACHE_LINE_SIZE*8));
#    endif
#    if LEAF_LINE_NUM >= 10
     pref (* ((char *)bbp + CACHE_LINE_SIZE*9));
#    endif
#    if LEAF_LINE_NUM >= 11
     pref (* ((char *)bbp + CACHE_LINE_SIZE*10));
#    endif
#    if LEAF_LINE_NUM >= 12
     pref (* ((char *)bbp + CACHE_LINE_SIZE*11));
#    endif
#    if LEAF_LINE_NUM >= 13
     pref (* ((char *)bbp + CACHE_LINE_SIZE*12));
#    endif
#    if LEAF_LINE_NUM >= 14
     pref (* ((char *)bbp + CACHE_LINE_SIZE*13));
#    endif
#    if LEAF_LINE_NUM >= 15
     pref (* ((char *)bbp + CACHE_LINE_SIZE*14));
#    endif
#    if LEAF_LINE_NUM >= 16
     pref (* ((char *)bbp + CACHE_LINE_SIZE*15));
#    endif
#    if LEAF_LINE_NUM >= 17
     pref (* ((char *)bbp + CACHE_LINE_SIZE*16));
#    endif
#    if LEAF_LINE_NUM >= 18
     pref (* ((char *)bbp + CACHE_LINE_SIZE*17));
#    endif
#    if LEAF_LINE_NUM >= 19
     pref (* ((char *)bbp + CACHE_LINE_SIZE*18));
#    endif
#    if LEAF_LINE_NUM >= 20
     pref (* ((char *)bbp + CACHE_LINE_SIZE*19));
#    endif
#    if LEAF_LINE_NUM >= 21
     pref (* ((char *)bbp + CACHE_LINE_SIZE*20));
#    endif
#    if LEAF_LINE_NUM >= 22
     pref (* ((char *)bbp + CACHE_LINE_SIZE*21));
#    endif
#    if LEAF_LINE_NUM >= 23
     pref (* ((char *)bbp + CACHE_LINE_SIZE*22));
#    endif
#    if LEAF_LINE_NUM >= 24
     pref (* ((char *)bbp + CACHE_LINE_SIZE*23));
#    endif
#    if LEAF_LINE_NUM >= 25
     pref (* ((char *)bbp + CACHE_LINE_SIZE*24));
#    endif
#    if LEAF_LINE_NUM >= 26
     pref (* ((char *)bbp + CACHE_LINE_SIZE*25));
#    endif
#    if LEAF_LINE_NUM >= 27
     pref (* ((char *)bbp + CACHE_LINE_SIZE*26));
#    endif
#    if LEAF_LINE_NUM >= 28
     pref (* ((char *)bbp + CACHE_LINE_SIZE*27));
#    endif
#    if LEAF_LINE_NUM >= 29
     pref (* ((char *)bbp + CACHE_LINE_SIZE*28));
#    endif
#    if LEAF_LINE_NUM >= 30
     pref (* ((char *)bbp + CACHE_LINE_SIZE*29));
#    endif
#    if LEAF_LINE_NUM >= 31
     pref (* ((char *)bbp + CACHE_LINE_SIZE*30));
#    endif
#    if LEAF_LINE_NUM >= 32
     pref (* ((char *)bbp + CACHE_LINE_SIZE*31));
#    endif
#    if LEAF_LINE_NUM >= 33
#    error "LEAF_LINE_NUM must be <= 32!"
#    endif
}

#define NODE_PREF_ST        NODE_PREF
#define LEAF_PREF_ST        LEAF_PREF

/* ---------------------------------------------------------------------- */
#ifndef clear_cache

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

static inline void software_clear_cache (void)
{char *buf;
 int   i;
        buf = (char *)memalign(CACHE_LINE_SIZE, 100*MB);
        if (buf == NULL) {
          perror ("malloc in clear_cache()");
          exit (1);
        }
        for (i=0; i<10*MB; i+=CACHE_LINE_SIZE)     /* write */
           buf[i] = 'a';
        for (i=0; i<10*MB; i+=CACHE_LINE_SIZE)     /* write */
           if (buf[i] != 'a') {
               printf ("Oops? clear_cache()\n");
               exit(1);
           }
        free (buf);
}

#define clear_cache                     software_clear_cache

#endif /* clear_cache */

/* ---------------------------------------------------------------------- */
#endif /* _BTREE_NODEPREF_H */
