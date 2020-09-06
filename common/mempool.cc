/**
 * @file mempool.cc
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 *
 * TBD
 *
 * @section DESCRIPTION
 *
 * The mempool class implements a memory pool for experiments purpose.  It will
 * allocate a contiguous region of DRAM from OS.  Then it will serve memory
 * allocation requests from this memory pool.  Freed btree nodes will be
 * appended into a linked list for future reuse.
 *
 * The memory pool is divided into segments.  Each worker thread has its own 
 * segment of the memory pool to reduce contention.
 */

#include "mempool.h"

thread_local int worker_id= -1;  /* in Thread Local Storage */

threadMemPools   the_thread_mempools;
threadNVMPools   the_thread_nvmpools;

/* -------------------------------------------------------------- */
void threadMemPools::init (int num_workers, long long size, long long align)
{
    assert((num_workers>0)&&(size>0)&&(align>0)&((align&(align-1))==0));

    // 1. allocate memory
    tm_num_workers= num_workers;
    tm_pools= new mempool[tm_num_workers];

    long long size_per_pool= (size/tm_num_workers/align)*align;
    size_per_pool= (size_per_pool<MB ? MB:size_per_pool);
    tm_size= size_per_pool*tm_num_workers;

    tm_buf= (char *)memalign (align, tm_size);
    if (!tm_buf || !tm_pools) {
	perror ("malloc"); exit (1);
    }

    // 2. initialize memory pools
    char name[80];
    for (int i=0; i<tm_num_workers; i++) {
       sprintf(name, "DRAM pool %d", i);
       tm_pools[i].init(tm_buf+i*size_per_pool, size_per_pool, align, strdup(name));
    }

    // 3. touch every page to make sure that they are allocated
    for(long long i = 0; i<tm_size; i+=4096) {
        tm_buf[i] = 1;
    }
}

void threadMemPools::print(void)
{
    if (tm_pools==NULL) {
      printf("Error: threadMemPools is not yet initialized!\n");
      return;
    }

    printf("threadMemPools\n");
    printf("--------------------\n");
    for (int i=0; i<tm_num_workers; i++) {
       tm_pools[i].print_params();
       tm_pools[i].print_free_nodes();
       printf("--------------------\n");
    }
}

void threadMemPools::print_usage(void)
{
    printf("threadMemPools\n");
    printf("--------------------\n");
    for (int i=0; i<tm_num_workers; i++) {
       tm_pools[i].print_usage();
    }
    printf("--------------------\n");
}

/* -------------------------------------------------------------- */
threadNVMPools::~threadNVMPools()
{
    if(tm_buf) {
#ifdef NVMPOOL_REAL
        pmem_unmap(tm_buf, tm_size);
#else  // NVMPOOL_REAL not defined, use DRAM memory
        free(tm_buf);
#endif
        tm_buf= NULL;
    }
    if(tm_pools) {
        delete[] tm_pools;
        tm_pools= NULL;
    }
}

/**
 * get the sigbus signal
 */
static void handleSigbus (int sig)
{
    printf("SIGBUS %d is received\n",sig);
    _exit(0);
}


void threadNVMPools::init (int num_workers, const char * nvm_file, long long size)
{
    // map_addr must be 4KB aligned, size must be multiple of 4KB
    assert((num_workers>0)&&(size>0)&&(size % 4096 == 0));

    // set sigbus handler
    signal(SIGBUS, handleSigbus);

    // 1. allocate memory
    tm_num_workers= num_workers;
    tm_pools= new mempool[tm_num_workers];
    if (!tm_pools) { perror ("malloc"); exit (1); }

    tn_nvm_file= nvm_file;

    long long size_per_pool= (size/tm_num_workers/4096)*4096;
    size_per_pool= (size_per_pool<MB ? MB:size_per_pool);
    tm_size= size_per_pool*tm_num_workers;


#ifdef NVMPOOL_REAL

    // pmdk allows PMEM_MMAP_HINT=map_addr to set the map address

    int is_pmem = false;
    size_t mapped_len = tm_size;

    tm_buf= (char *) pmem_map_file(tn_nvm_file, tm_size, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
    if (tm_buf == NULL) {
       perror ("pmem_map_file");
       exit(1);
    }

    printf("NVM mapping address: %p, size: %ld\n", tm_buf, mapped_len);
    if (tm_size != mapped_len) {
       fprintf(stderr, "Error: cannot map %lld bytes\n", tm_size);
       pmem_unmap(tm_buf, mapped_len);
       exit(1);
    }

#else  // NVMPOOL_REAL not defined, use DRAM memory

    tm_buf= (char *)memalign (4096, tm_size);
    if (!tm_buf) {
        perror ("malloc"); exit (1);
    }

#endif // NVMPOOL_REAL

    // 2. initialize NVM memory pools
    char name[80];
    for (int i=0; i<tm_num_workers; i++) {
       sprintf(name, "NVM pool %d", i);
       tm_pools[i].init(tm_buf+i*size_per_pool, size_per_pool, 4096, strdup(name));
    }

    // 3. touch every page to make sure that they are allocated
    for(long long i = 0; i<tm_size; i+=4096) {
        tm_buf[i] = 1;  // XXX: need a special signature
    }
}


void threadNVMPools::print(void)
{
    if (tm_pools==NULL) {
      printf("Error: threadNVMPools is not yet initialized!\n");
      return;
    }

    printf("threadNVMPools\n");
    printf("--------------------\n");
    for (int i=0; i<tm_num_workers; i++) {
       tm_pools[i].print_params();
       tm_pools[i].print_free_nodes();
       printf("--------------------\n");
    }
}

void threadNVMPools::print_usage(void)
{
    printf("threadNVMPools\n");
    printf("--------------------\n");
    for (int i=0; i<tm_num_workers; i++) {
       tm_pools[i].print_usage();
    }
    printf("--------------------\n");
}

#if 0
/* test */

#define mempool_alloc       the_mempool.alloc
#define mempool_free        the_mempool.free
#define mempool_alloc_node  the_mempool.alloc_node
#define mempool_free_node   the_mempool.free_node


/* single thread small pool */
void test1()
{
    the_thread_mempools.init(1, 2*1024*1024, 64); 
    worker_id= 0;

    // 1. after init
    printf("After initialization\n");
    the_thread_mempools.print();

    // 2. alloc and free
    char *p= (char *)mempool_alloc(1024);
    printf("alloc(1024)= %p\n", p);

    mempool_free(p);
    printf("free should do nothing\n");

    the_thread_mempools.print();

    // 3. alloc_node, free_node
    char *pnode[5];
    for (int i=0; i<5; i++) {
       pnode[i]= (char *)mempool_alloc_node(64);
       printf("mempool_alloc_node(64)=%p\n", pnode[i]);
    }
    printf("free all allocated nodes\n");
    for (int i=0; i<5; i++) {
       mempool_free_node(pnode[i]);
    }

    the_thread_mempools.print();

    // 4. alloc node again
    for (int i=0; i<3; i++) {
       pnode[i]= (char *)mempool_alloc_node(64);
       printf("mempool_alloc_node(64)=%p\n", pnode[i]);
    }

    the_thread_mempools.print();
}

#include <pthread.h>

void * test2ThMain(void *arg)
{
    worker_id= (int) (long long) arg;

    // 1. alloc and free
    char *p= (char *)mempool_alloc(worker_id*64);
    printf("alloc(%d)= %p\n", worker_id*64, p);

    // 3. alloc_node, free_node
    char *pnode[5];
    for (int i=0; i<5; i++) {
       pnode[i]= (char *)mempool_alloc_node(64);
       printf("worker %d mempool_alloc_node(64)=%p\n", worker_id, pnode[i]);
    }
    printf("worker %d free all allocated nodes\n", worker_id);
    for (int i=0; i<5; i++) {
       mempool_free_node(pnode[i]);
    }

    // 4. alloc node again
    for (int i=0; i<3; i++) {
       pnode[i]= (char *)mempool_alloc_node(64);
       printf("worker %d mempool_alloc_node(64)=%p\n", worker_id, pnode[i]);
    }

    return NULL;
}

// 3 threads, each allocating and freeing nodes.
void test2()
{
    pthread_t tid[3];

    the_thread_mempools.init(3, 2*1024*1024, 64);  // 3 workers
    the_thread_mempools.print();
    
    for (int i=0; i<3; i++) {
       if (pthread_create(&(tid[i]), NULL, test2ThMain, (void *)(long long)i) != 0) {
           fprintf(stderr, "pthread_create error!\n"); exit(1);
       }
    }
    for (int i=0; i<3; i++) {
       void *retval= NULL;
       pthread_join(tid[i], &retval);
    }
    the_thread_mempools.print();
}

void * test3ThMain(void *arg)
{
    worker_id= (int) (long long) arg;

    // 1. alloc and free
    char *p= (char *)nvmpool_alloc(worker_id*64);
    printf("alloc(%d)= %p\n", worker_id*64, p);

    // 3. alloc_node, free_node
    char *pnode[5];
    for (int i=0; i<5; i++) {
       pnode[i]= (char *)nvmpool_alloc_node(64);
       printf("worker %d nvmpool_alloc_node(64)=%p\n", worker_id, pnode[i]);
    }
    printf("worker %d free all allocated nodes\n", worker_id);
    for (int i=0; i<5; i++) {
       nvmpool_free_node(pnode[i]);
    }

    // 4. alloc node again
    for (int i=0; i<3; i++) {
       pnode[i]= (char *)nvmpool_alloc_node(64);
       printf("worker %d nvmpool_alloc_node(64)=%p\n", worker_id, pnode[i]);
    }

    return NULL;
}

// nvm pool
void test3()
{
    pthread_t tid[3];

    printf("test3\n");

    //char line[80];
    //sprintf(line, "cat /proc/%d/maps", getpid());
    //if (system(line) < 0) { 
    //   perror("system");
    //}

    the_thread_nvmpools.init(3, "/mnt/mypmem0/chensm/leafdata", MB);
    the_thread_nvmpools.print();
    
    for (int i=0; i<3; i++) {
       if (pthread_create(&(tid[i]), NULL, test3ThMain, (void *)(long long)i) != 0) {
           fprintf(stderr, "pthread_create error!\n"); exit(1);
       }
    }
    for (int i=0; i<3; i++) {
       void *retval= NULL;
       pthread_join(tid[i], &retval);
    }
    the_thread_nvmpools.print();
}

int main()
{
    test3();
    return 0;
}

/* compile

$ g++ -O3 -std=c++11 -pthread mempool.cc -lpmem -o dbg-mempool
 
 */

#endif // 1 to enable, 0 to disable
