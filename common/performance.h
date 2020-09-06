/* File Name: performance.h
 * Author:    Shimin Chen, Jihang Liu, Leying Chen
 */

#ifndef _BTREE_PERFORMANCE_H
#define _BTREE_PERFORMANCE_H

#include <sys/time.h>
#include <unistd.h>

#define  TEST_PERFORMANCE(ret, COMMAND)                 \
{ struct timeval _start_tv, _end_tv;                    \
  long long total_us;                                   \
                                                        \
   if (gettimeofday (&_start_tv, NULL) < 0) {           \
     perror ("get start time");                         \
   }                                                    \
                                                        \
   COMMAND;                                             \
                                                        \
   if (gettimeofday (&_end_tv, NULL) < 0) {             \
     perror ("get end time");                           \
   }                                                    \
                                                        \
   total_us = (_end_tv.tv_sec - _start_tv.tv_sec)*1000000\
             +(_end_tv.tv_usec- _start_tv.tv_usec);     \
   printf ("elapsed time:%lld us\n", total_us);         \
   (ret) = total_us;                                    \
}

// -----------------------------------------------------------------------------
#endif /* _BTREE_PERFORMANCE_H */
