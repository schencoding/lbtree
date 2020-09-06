/**
 * @file dbg-nvmlog-log.cc
 * @author  Shimin Chen <shimin.chen@gmail.com>
 * @version 1.0
 *
 * @section LICENSE
 * 
 * TBD
 *       
 * @section DESCRIPTION
 *
 * check the correctness of class NvmLog_Log
 *
 * compilation:
 *    g++ -O3 -std=c++11 -pthread -I../ ../mempool.cc dbg-nvmlog-log.cc -lpmem -o dbg-nvmloglog
 */

#define NVM_LOG_SIZE   256  // 4 cache lines

#include "nvm-common.h"

NvmLog_Log  mylogbuf;
NvmLog_Log::nl_log_pointer_t readpos;

char buf[1024];
int  len;

int main()
{
    the_thread_nvmpools.init(3, "/mnt/mypmem0/chensm/leafdata", MB);
    worker_id= 0;

    mylogbuf.initLog();
    mylogbuf.prepareLogforWriting();
    mylogbuf.getLogCurPos(&readpos);

    while(1) {
        mylogbuf.printLog();
        mylogbuf.printLogWritePos();
        mylogbuf.printLogReadPos(&readpos);

        int cmd, retval;
        printf("\n0.exit 1.prepareLogforWriting 2.writeLog 3.flushLog\n" 
          "4.getLogCurPos 5.prepareForRead 6.readLog 7.readLogSkip\n"
          "8.prepareForReverseRead 9.readLogReverse 10.readLogReverseSkip\n");
        if(scanf("%d", &cmd)!=1) continue;

        if (cmd ==0) break;

        switch(cmd) {
        case 1:
                mylogbuf.prepareLogforWriting();
                break;
        case 2:
                do {
                   printf("record size: ");
                } while (scanf("%d", &len) != 1);
                for(int ii=0; ii<len; ii++) buf[ii]=len;
                mylogbuf.writeLog(buf, len);
                break;
        case 3:
                mylogbuf.flushLog();
                break;
        case 4:
                mylogbuf.getLogCurPos(&readpos);
                break;
        case 5:
                mylogbuf.prepareForRead(&readpos);
                break;
        case 6:
                do {
                   printf("record size: ");
                } while (scanf("%d", &len) != 1);
                retval= mylogbuf.readLog(&readpos, buf, len);
                printf("return %d bytes:\n", retval);
                for (int ii=0; ii<retval; ii++) {
                   printf(" %2x", (unsigned char)buf[ii]);
                   if (ii % 16 == 15) printf("\n");
                }
                printf("\n");
                break;
        case 7:
                do {
                   printf("record size: ");
                } while (scanf("%d", &len) != 1);
                retval= mylogbuf.readLogSkip(&readpos, len);
                printf("skip %d bytes\n", retval);
                break;
        case 8:
                mylogbuf.prepareForReverseRead(&readpos);
                break;
        case 9:
                do {
                   printf("record size: ");
                } while (scanf("%d", &len) != 1);
                retval= mylogbuf.readLogReverse(&readpos, buf, len);
                printf("return %d bytes:\n", retval);
                for (int ii=len-retval; ii<len; ii++) {
                   printf(" %2x", (unsigned char)buf[ii]);
                   if (ii % 16 == 15) printf("\n");
                }
                printf("\n");
                break;
        case 10:
                do {
                   printf("record size: ");
                } while (scanf("%d", &len) != 1);
                retval= mylogbuf.readLogReverseSkip(&readpos, len);
                printf("skip %d bytes\n", retval);
                break;
        }

    } // while

    return 0;
}
