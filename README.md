## Introduction

This directory contains code for the following paper:

   **Jihang Liu, Shimin Chen**. *LB+-Trees: Optimizing Persistent Index Performance on 3DXPoint Memory*. PVLDB 13(7): 1078-1090. 


## Machine Configuration

The code is designed for Machines equipped with Intel Optane DC Persistent Memory.  It uses PMDK to map NVM into the virtual address space of the process.  It also uses Intel RTM for concurrency control purpose.

Suppose the NVDIMM device is /dev/pmem0, a file system is created on the device, and it is mounted to /mnt/mypmem0. Make sure we have permissions to create and read files in /mnt/mypmem0.  For example, we can create a subdirectory for each user that wants to work on NVM, then use chown to change the owner of the subdirectory to the user.

Next, create a file on NVM for storing the leaf nodes of the B+-trees:

    $ dd if=/dev/zero of=/mnt/mypmem0/user/filename bs=1048576 count=num-MB

## Directory Structure

The directory is organized as follows:

Name         | Explanation
------------ | -------------
Makefile     | used to compile the code for the tree implementation
MyCheckTree.sh | simple correctness checks
common       | common headers and main driver program
lbtree-src   | LB+-Tree
fptree-src   | FP-Tree
wbtree-src   | WB+-Tree
keygen-8B    | generate 8B keys for experiments

## Compilation

```
$ make
```

## Command Line Options

```
Usage: ./lbtree [<command> <params>] ...
--------------------------------------------------
[Initialization]
 thread must be the first command, followed by mempool and nvmpool.

   thread  <worker_thread_num>
   mempool <size(MB)>
   nvmpool <filename> <size(MB)>
--------------------------------------------------
[Debugging]
 use these commands to test the correctness of the implementation

   debug_bulkload <key_num> <fill_factor>
   debug_randomize <key_num> <fill_factor>
   debug_lookup <key_num> <fill_factor>
   debug_insert <key_num>
   debug_del <key_num>
--------------------------------------------------
[Test Preparation]
 prepare a tree before performance tests

   bulkload <key_num> <key_file> <fill_factor>
   randomize
   stable <key_num> <key_file>
--------------------------------------------------
[Performance Tests]
 measure performance of various tree operations

   lookup <key_num> <key_file>
   insert <key_num> <key_file>
   del <key_num> <key_file>
--------------------------------------------------
[Misc]
 helper commands. debug_test enables correctness check for performance tests.

   print_tree
   check_tree
   print_mem
   debug_test
   sleep <seconds>
--------------------------------------------------
```

## Simple Correctness Check    

MyCheckTree.sh contains a number of example debugging tests.
    
```
$ ./MyCheckTree.sh ./lbtree
debug_bulkload
Test  1: bulkload is good!
Test  2: bulkload is good!
Test  3: bulkload is good!
Test  4: bulkload is good!
debug_insert
Test  5: insertion is good!
Test  6: insertion is good!
Test  7: insertion is good!
Test  8: insertion is good!
Test  9: insertion is good!
debug_del
Test 10: delete is good!
Test 11: delete is good!
Test 12: delete is good!
Test 13: delete is good!
Test 14: delete is good!
debug_lookup
Test 15: lookup is good!
Test 16: lookup is good!
Test 17: lookup is good!
Test 18: lookup is good!
```

## Generate Keys for Experiments

```
$ cd keygen-8B/
$ make
$ ./mygen.sh
```

mygen.sh generates 50K bulkload keys, 500 random search keys, 500 insert keys, and 500 delete keys.  The tools guarantee that the insert keys are not in bulkload keys and the delete keys are all in bulkload keys.  In this way, all insertions and deletions will succeed.

mygen.sh can be modified to generate the desired test keys.

## Test Runs
1. Set up optional environment variables  

PMEM_MMAP_HINT can control the NVM mapping address in PMDK.  For simplicity, we also set NVMFILE to be the NVM file path.
```
$ export NVMFILE="/mnt/mypmem0/chensm/leafdata"
$ export PMEM_MMAP_HINT="0x600000000000"
```
2. Lookup test

We bulkload 50K keys to make the tree 100% full.  Then we performs 500 random lookups.  The number of worker threads is 2.  Both bulkload and lookup use 2 threads.  Leaf nodes are in the NVM pool, which is 200MB in total (and 200MB/2 per thread).  Similarly, non-leaf nodes are in the DRAM pool, which is 100MB in total (and 100MB/2 per thread).
```
$ ./lbtree thread 2 mempool 100 nvmpool ${NVMFILE} 200 bulkload 50000 keygen-8B/dbg-k50k 1.0 lookup 500 keygen-8B/dbg-lookup500
```

3. Insert test

We bulkload 50K keys to make the tree 100% full.  Then we performs 500 random insertions.
```
$ ./lbtree thread 2 mempool 100 nvmpool ${NVMFILE} 200 bulkload 50000 keygen-8B/dbg-k50k 1.0 insert 500 keygen-8B/dbg-insert500
```

4. Delete test

We bulkload 50K keys to make the tree 100% full.  Then we performs 500 random deletions.
```
$ ./lbtree thread 2 mempool 100 nvmpool ${NVMFILE} 200 bulkload 50000 keygen-8B/dbg-k50k 1.0 del 500 keygen-8B/dbg-del500
```