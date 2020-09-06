/**
 * @file nvmlog-common.cc
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 * 
 * TBD
 *       
 * @section DESCRIPTION
 *
 * initliaze per-worker log
 */

#include "nvm-common.h"

NvmLog *the_nvm_logs;

void nvmLogInit(int num_workers)
{
    the_nvm_logs = new NvmLog[num_workers];
    if (!the_nvm_logs) {
      perror("new"); exit(1);
    }
}
