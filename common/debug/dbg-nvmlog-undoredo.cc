/**
 * @file dbg-nvmlog-undoredo.cc
 * @author  Shimin Chen <shimin.chen@gmail.com>
 * @version 1.0
 *
 * @section LICENSE
 * 
 * TBD
 *       
 * @section DESCRIPTION
 *
 * check the correctness of class NvmLog
 *
 * compilation:
 *    g++ -O3 -std=c++11 -pthread -I../ ../mempool.cc dbg-nvmlog-undoredo.cc -lpmem -o dbg-nvmlog
 */

#define NVM_LOG_SIZE   1024 

#include "nvm-common.h"

NvmLog mylog;

unsigned long long  i8B=8, j8B=0x18, k8B=0x48;
unsigned int        i4B=4, j4B=0x14, k4B=0x44;
unsigned short      i2B=2, j2B=0x12, k2B=0x42;
unsigned char       i1B=1, j1B=0x11, k1B=0x41;
char                ibuf[128]= "good morning";
char                jbuf[128]= "good afternoon";
char                kbuf[128]= "good evening";

void               *node[8];

int main()
{
    the_thread_nvmpools.init(3, "/mnt/mypmem0/chensm/leafdata", MB);
    worker_id= 0;

    mylog.init();

    printf("begin\n");
    mylog.startMiniTransaction();

    printf ("mylog.write8B(&i8B, 0x28)\n");
    mylog.write8B(&i8B, 0x28);
    node[0]= mylog.allocNode(64);
    mylog.delNode(node[0]);

    printf ("mylog.write4B(&i4B, 0x24)\n");
    mylog.write4B(&i4B, 0x24);
    node[1]= mylog.allocNode(64);
    mylog.delNode(node[1]);

    printf ("mylog.write2B(&i2B, 0x22)\n");
    mylog.write2B(&i2B, 0x22);
    node[2]= mylog.allocNode(64);
    mylog.delNode(node[2]);

    printf ("mylog.write1B(&i1B, 0x21)\n");
    mylog.write1B(&i1B, 0x21);
    node[3]= mylog.allocNode(64);
    mylog.delNode(node[3]);

    printf ("mylog.writeVchar(ibuf, 12, GOOD MORNING)\n");
    mylog.writeVchar(ibuf, 12, "GOOD MORNING");
    node[4]= mylog.allocNode(64);
    mylog.delNode(node[4]);

    printf ("mylog.new8B(&j8B, 0x38)\n");
    mylog.new8B(&j8B, 0x38);
    node[5]= mylog.allocNode(64);
    mylog.delNode(node[5]);

    printf ("mylog.new4B(&j4B, 0x34)\n");
    mylog.new4B(&j4B, 0x34);
    node[6]= mylog.allocNode(64);
    mylog.delNode(node[6]);

    printf ("mylog.new2B(&j2B, 0x32)\n");
    mylog.new2B(&j2B, 0x32);
    node[7]= mylog.allocNode(64);
    mylog.delNode(node[7]);

    printf ("mylog.new1B(&j1B, 0x31)\n");
    mylog.new1B(&j1B, 0x31);

    printf ("mylog.newVchar(jbuf, 14, GOOD AFTERNOON)\n");
    mylog.newVchar(jbuf, 14, "GOOD AFTERNOON");

    printf ("mylog.redoWrite8B(&k8B, 0x58)\n");
    mylog.redoWrite8B(&k8B, 0x58);

    printf ("mylog.redoWrite4B(&k4B, 0x54)\n");
    mylog.redoWrite4B(&k4B, 0x54);

    printf ("mylog.redoWrite2B(&k2B, 0x52)\n");
    mylog.redoWrite2B(&k2B, 0x52);

    printf ("mylog.redoWrite1B(&k1B, 0x51)\n");
    mylog.redoWrite1B(&k1B, 0x51);

    printf ("mylog.redoWriteVchar(kbuf, 12, GOOD EVENING)\n");
    mylog.redoWriteVchar(kbuf, 12, "GOOD EVENING");
#if 0
#endif

    printf ("(1) commit (0) abort :");
    int choice;
    while (scanf("%d", &choice) != 1);

    if (choice == 1) {
       printf ("mylog.commitMiniTransaction()\n");
       mylog.commitMiniTransaction();
    }
    else {
       printf ("mylog.abortMiniTransaction()\n");
       mylog.abortMiniTransaction();
    }

    // mylog.nl_logbuf_.printLog();
    mylog.print();


    printf("i8B=%llx, j8B=%llx, k8B=%llx\n", i8B, j8B, k8B);
    printf("i4B=%x, j4B=%x, k4B=%x\n", i4B, j4B, k4B);
    printf("i2B=%x, j2B=%x, k2B=%x\n", i2B, j2B, k2B);
    printf("i1B=%x, j1B=%x, k1B=%x\n", i1B, j1B, k1B);
    printf("ibuf=%s, jbuf=%s, kbuf=%s\n", ibuf, jbuf, kbuf);

    return 0;
}
