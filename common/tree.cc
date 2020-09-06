/**
 * @file tree.cc
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 *
 * TBD
 * 
 * @section DESCRIPTION
 *      
 * This file contains the main driver for experiments.
 */
   
#include "tree.h"

/* ------------------------------------------------------------------------ */
/*               global variables                                           */
/* ------------------------------------------------------------------------ */
tree *       the_treep= NULL;
int          worker_thread_num= 0;
const char * nvm_file_name= NULL;
bool         debug_test= false;

#ifdef INSTRUMENT_INSERTION
int insert_total;	// insert_total=
int insert_no_split;    //              insert_no_split
int insert_leaf_split;  //             +insert_leaf_split
int insert_nonleaf_split;//            +insert_nonleaf_split
int total_node_splits;  // an insertion may cause multiple node splits
#endif // INSTRUMENT_INSERTION

#ifdef NVMFLUSH_STAT
NVMFLUSH_STAT_DEFS;
#endif

/* ------------------------------------------------------------------------ */
/*               get keys from a key file of int keys                       */
/* ------------------------------------------------------------------------ */
static Int64 * getKeys (char *filename, Int64 num)
{
    int fd;
    Int64 *p;
   fd = open (filename, 0, 0600);
   if (fd == -1) {
     perror (filename); exit(1);
   }

   p = (Int64 *) malloc ((num+1) * sizeof(Int64));	
   if (p == NULL) {
     printf ("malloc error\n"); exit (1);
   }
   // we allocate one more element at the end so that key[num] does not
   // cause segfault.  This simplifies our program in main()

   if (read (fd, p, num*sizeof(Int64)) != num * sizeof(Int64))
     {perror ("read"); exit (1);}

   close (fd);
   return p;
}

/* ------------------------------------------------------------------------ */
/*               command line usage                                         */
/* ------------------------------------------------------------------------ */
static void usage (char *cmd)
{
  fprintf (stderr, 
        "Usage: %s [<command> <params>] ... \n"
        "--------------------------------------------------\n"
        "[Initialization]\n"
        " thread must be the first command, followed by mempool and nvmpool.\n\n"
        "   thread  <worker_thread_num>\n"
        "   mempool <size(MB)>\n"
        "   nvmpool <filename> <size(MB)>\n"
        "--------------------------------------------------\n"
        "[Debugging]\n"
        " use these commands to test the correctness of the implementation\n\n"
        "   debug_bulkload <key_num> <fill_factor>\n"
        "   debug_randomize <key_num> <fill_factor>\n"
        "   debug_lookup <key_num> <fill_factor>\n"
        "   debug_insert <key_num>\n"
        "   debug_del <key_num>\n"
        "--------------------------------------------------\n"
        "[Test Preparation]\n"
        " prepare a tree before performance tests\n\n"
        "   bulkload <key_num> <key_file> <fill_factor>\n"
        "   randomize\n"
        "   stable <key_num> <key_file>\n"
        "--------------------------------------------------\n"
        "[Performance Tests]\n"
        " measure performance of various tree operations\n\n"
        "   lookup <key_num> <key_file>\n"
        "   insert <key_num> <key_file>\n"
        "   del <key_num> <key_file>\n"
        "--------------------------------------------------\n"
        "[Misc]\n"
        " helper commands. debug_test enables correctness check for performance tests.\n\n"
        "   print_tree\n"
        "   check_tree\n"
        "   print_mem\n"
        "   debug_test\n"
        "   sleep <seconds>\n"
        "--------------------------------------------------\n"
        , cmd);
  exit (1);
}

int compar_void_ptr(const void *p1, const void *p2)
{
	long *v1 = (long *)p1;
	long *v2 = (long *)p2;
        long t= (*v1 - *v2);

	return (t>0 ? 1 : (t<0 ? -1 : 0));
}

/**
 * The test run for lookup operations
 */
static inline int lookupTest(Int64 key[], int start, int end)
{
       int found= 0;

       for (int ii=start; ii<end; ii++) {

          void *p;
          int pos;
          p = the_treep->lookup (key[ii], &pos);

          if (debug_test) {
            if (pos >= 0) { // found
               void * recptr = the_treep->get_recptr (p, pos);
               assert ((key_type)recptr == key[ii]); // ptr == key in test prep
               found ++;
            }
          }

       } // end of for

       return found;
}

/**
 * The test run for insert operations
 */
static inline int insertTest(Int64 key[], int start, int end)
{
       int found= 0;

       for (int ii=start; ii<end; ii++) {
          key_type kk= key[ii];
          the_treep->insert (kk, (void *) kk);
       } // end of for

       if (debug_test) {
          for (int ii=start; ii<end; ii++) {
             void *p;
             int pos;
             p = the_treep->lookup (key[ii], &pos);
             found += (pos >= 0);
          }
       }

       return found;
}

/**
 * The test run for deletion operations
 */
static inline int delTest(Int64 key[], int start, int end)
{
       int found= 0;

       for (int ii=start; ii<end; ii++) {
          key_type kk= key[ii];
	  the_treep->del (kk);
       } // end of for

       if (debug_test) {
          for (int ii=start; ii<end; ii++) {
             void *p;
             int pos;
             p = the_treep->lookup (key[ii], &pos);
             found += (pos >= 0);
          }
       }

       return found;
}


int parse_command (int argc, char **argv)
{
	if (argc < 2) usage (argv[0]);
	char * cmd = argv[0];
	argc --; argv ++;

	while (argc > 0) {

          // *****************************************************************
          // Initialization
          // *****************************************************************
          
	  // ---
	  // thread  <worker_thread_num>
	  // ---
	  if (strcmp(argv[0],"thread") == 0){
	    if(argc < 2) usage(cmd);

	    worker_thread_num = atoi(argv[1]);
            worker_id= 0; // the main thread will use worker[0]'s mem/nvm pool

	    printf("number of worker threads is %d\n", worker_thread_num);
	    argc -= 2; argv += 2;
	  }

          // ---
          // mempool <size(MB)>
          // ---
          else if (strcmp (argv[0], "mempool") == 0) {
            // get params    
            if (argc < 2) usage (cmd);
            long long size = atoi(argv[1]);
            size *= MB;
            argc -= 2; argv += 2;

            if (worker_thread_num <= 0) {
                fprintf(stderr, "need to set worker_thread_num first!\n");
                exit(1);
            }

            // initialize mempool per worker thread
            the_thread_mempools.init(worker_thread_num, size, 4096);
          }

	  // ---
	  // nvmpool <filename> <size(MB)>
	  // ---
	  else if(strcmp(argv[0], "nvmpool") == 0){
            // get params
	    if(argc < 3) usage(cmd);
	    nvm_file_name = argv[1];
	    long long size = atoi(argv[2]);
	    size *= MB;
	    argc -= 3; argv += 3;

            if (worker_thread_num <= 0) {
                fprintf(stderr, "need to set worker_thread_num first!\n");
                exit(1);
            }

            // initialize nvm pool and log per worker thread
            the_thread_nvmpools.init(worker_thread_num, nvm_file_name, size);

            // allocate a 4KB page for the tree in worker 0's pool
            char *nvm_addr= (char *)nvmpool_alloc(4*KB);
            the_treep= initTree(nvm_addr, false);

            // log may not be necessary for some tree implementations
            // For simplicity, we just initialize logs.  This cost is low.
            nvmLogInit(worker_thread_num);
	  }

          // *****************************************************************
          // Misc
          // *****************************************************************

          // ---
          // print_tree
          // ---
          else if (strcmp (argv[0], "print_tree") == 0) {
            // get params
            argc -= 1; argv += 1;

	    the_treep->print ();
          }

          // ---
          // check_tree
          // ---
          else if (strcmp (argv[0], "check_tree") == 0) {
            // get params
            argc -= 1; argv += 1;

            key_type start, end;
            the_treep->check (&start, &end);
            printf("Check tree structure OK\n");
          }

          // ---
          // print_mem
          // ---
          else if (strcmp (argv[0], "print_mem") == 0) {
            // get params
            argc -= 1; argv += 1;

            the_thread_mempools.print_usage();
            the_thread_nvmpools.print_usage();
          }

          // ---
          // debug_test
          // ---
          else if (strcmp (argv[0], "debug_test") == 0) {
            // get params
            argc -= 1; argv += 1;

	    debug_test= true;
          }

          // ---
          // sleep <seconds>
          // ---
          else if (strcmp (argv[0], "sleep") == 0) {
            // get params
            if (argc < 2) usage (cmd);
            int seconds= atoi(argv[1]);
            argc -= 2; argv += 2;

	    printf("sleep %d seconds\n", seconds);
            sleep(seconds);
          } 

          // *****************************************************************
          // Debugging
          // *****************************************************************
          
	  // ---
	  // debug_bulkload <key_num> <fill_factor>
	  // ---
          else if (strcmp (argv[0], "debug_bulkload") == 0) {
	    // get params    
	    if (argc < 3) usage (cmd);
	    int keynum = atoi (argv[1]);
	    float bfill; sscanf (argv[2], "%f", &bfill);
	    argc -= 3; argv += 3;

            // initiate keys
	    simpleKeyInput *input = new simpleKeyInput(2*keynum, 0, 2);

	    // bulkload then check
	    int level = the_treep->bulkload (keynum, input, bfill);
	    printf ("root is at %d level\n", level);

	    key_type start, end;
	    the_treep->check (&start, &end);

	    assert ((start == input->get_key(0)) 
		 && (end == input->get_key(keynum-1)));

	    // free keys
            delete input;

	    printf ("bulkload is good!\n");
	  }

          // ---
          // debug_randomize <key_num> <fill_factor>
          // ---
          else if (strcmp (argv[0], "debug_randomize") == 0) {
            // get params    
            if (argc < 3) usage (cmd);
            int keynum = atoi (argv[1]);
            float bfill; sscanf (argv[2], "%f", &bfill);
            argc -= 3; argv += 3;

            // initiate keys
	    simpleKeyInput *input = new simpleKeyInput(2*keynum, 0, 2);

            // bulkload then check
            int level = the_treep->bulkload (keynum, input, bfill);
            printf ("root is at %d level\n", level);
	    the_treep->randomize();  // randomize a sorted tree
	    the_treep->randomize();  // randomize an already random tree

            key_type start, end;
            the_treep->check (&start, &end);

	    assert ((start == input->get_key(0)) 
		 && (end == input->get_key(keynum-1)));

            // free keys
            delete input;

            printf ("randomize is good!\n");
          }

          // ---
          // debug_lookup <key_num> <fill_factor>
          // ---
          else if (strcmp (argv[0], "debug_lookup") == 0) {
            // get params
            if (argc < 3) usage (cmd);
            int keynum = atoi (argv[1]);
            float bfill; sscanf (argv[2], "%f", &bfill);
            argc -= 3; argv += 3;

            // initiate keys
	    inMemKeyInput *input = new inMemKeyInput(2*keynum, 1, 2);

            // bulkload then check
            int level = the_treep->bulkload (keynum, input, bfill);
            the_treep->randomize();

            key_type start, end;
            the_treep->check (&start, &end);

	    assert ((start == input->keys[1]) 
		 && (end == input->keys[2*keynum-1]));

	    // check look up
	    for (int ii=0; ii<keynum; ii++) {
	       void *p;
	       int pos;

               Int64 kk= input->keys[2*ii];
	       p = the_treep->lookup (kk, &pos);
	       if (pos >= 1)
	         assert ((key_type)(the_treep->get_recptr (p, pos)) != kk);

               kk= input->keys[2*ii+1];
	       p = the_treep->lookup (kk, &pos);
	       assert ((key_type)(the_treep->get_recptr (p, pos)) == kk);
	    }

            printf ("lookup is good!\n");
          }

          // ---
          // debug_insert <key_num>
          // ---
          else if (strcmp (argv[0], "debug_insert") == 0) {
            // get params
            if (argc < 2) usage (cmd);
            int keynum = atoi (argv[1]);
            float bfill=1.0;
            argc -= 2; argv += 2;

	    printf("test 1\n");

            {// initiate keys
	    inMemKeyInput *input = new inMemKeyInput(2*keynum, 1, 2);

             // bulkload 1 key
            int level = the_treep->bulkload (1, input, bfill);

            // insertion: keynum-1 keys
            int keys_per_thread= floor(keynum-1, worker_thread_num);

            // run tests with multiple threads
	    std::thread threads[worker_thread_num];
	    for (int t=0; t<worker_thread_num; t++){
		threads[t] = std::thread ( [=](){
                     worker_id= t;
                     int start= 1 + keys_per_thread*t;
                     int end= ((t < worker_thread_num-1)
                               ? start+keys_per_thread : keynum);
                     for (int ii=start; ii<end; ii++) {
                        key_type kk= input->keys[2*ii+1];
                        the_treep->insert (kk, (void *) kk);
		     }
		});
	    }
	    for (int t=0; t<worker_thread_num; t++) threads[t].join();

	    // check
            key_type start, end;
            the_treep->check (&start, &end);
	    assert ((start == input->keys[1]) 
		 && (end == input->keys[2*keynum-1]));

            for (int ii=0; ii<keynum; ii++) {
               void *p;
               int pos;
	       key_type kk;

	       kk= input->keys[2*ii];
               p = the_treep->lookup (kk, &pos);
	       if (pos >= 1)
               assert ((key_type)(the_treep->get_recptr (p, pos)) != kk);

	       kk= input->keys[2*ii+1];
               p = the_treep->lookup (kk, &pos);
               assert ((key_type)(the_treep->get_recptr (p, pos)) == kk);
            }

	    delete input;
	    }

	    printf("test 2\n");

            {// initiate keys
	    inMemKeyInput *input = new inMemKeyInput(2*keynum, 1, 2);

                // Hmm... we have not reclaimed the space of the old tree.
                // But it's ok for debugging.

            // bulkload odd keys
            int level = the_treep->bulkload (keynum, input, bfill);

            // randomize the leaf nodes
	    the_treep->randomize();

            // insertion: keynum even keys
            int keys_per_thread= floor(keynum, worker_thread_num);

            // run tests with multiple threads
	    std::thread threads[worker_thread_num];
	    for (int t=0; t<worker_thread_num; t++){
		threads[t] = std::thread ( [=](){
                     worker_id= t;
                     int start= keys_per_thread*t;
                     int end= ((t < worker_thread_num-1)
                               ? start+keys_per_thread : keynum);
                     for (int ii=start; ii<end; ii++) {
                        key_type kk= input->keys[2*ii];
                        the_treep->insert (kk, (void *) kk);
		     }
		});
	    }
	    for (int t=0; t<worker_thread_num; t++) threads[t].join();

            // check
            key_type start, end;
            the_treep->check (&start, &end);
	    assert ((start == input->keys[0]) 
		 && (end == input->keys[2*keynum-1]));

            // duplicate insertions: insert keys again
	    for (int t=0; t<worker_thread_num; t++){
		threads[t] = std::thread ( [=](){
                     worker_id= t;
                     int start= keys_per_thread*t;
                     int end= ((t < worker_thread_num-1)
                               ? start+keys_per_thread : keynum);
                     for (int ii=start; ii<end; ii++) {
                        key_type kk= input->keys[2*ii];
                        the_treep->insert (kk, (void *) kk);
		     }
		});
	    }
	    for (int t=0; t<worker_thread_num; t++) threads[t].join();

            // check
            start, end;
            the_treep->check (&start, &end);

	    assert ((start == input->keys[0]) 
		 && (end == input->keys[2*keynum-1]));

            for (int ii=0; ii<keynum; ii++) {
               void *p;
               int pos;
	       key_type kk;

	       kk= input->keys[2*ii];
               p = the_treep->lookup (kk, &pos);
               assert ((key_type)(the_treep->get_recptr (p, pos)) == kk);

	       kk= input->keys[2*ii+1];
               p = the_treep->lookup (kk, &pos);
               assert ((key_type)(the_treep->get_recptr (p, pos)) == kk);
            }
            }

            printf ("insertion is good!\n");
          }

          // ---
          // debug_del <key_num>
          // ---
          else if (strcmp (argv[0], "debug_del") == 0) {
            // get params
            if (argc < 2) usage (cmd);
            int keynum = atoi (argv[1]);
            float bfill=1.0;
            argc -= 2; argv += 2;

	    if (keynum < 10) keynum = 10;

            // initiate keys
	    inMemKeyInput *input = new inMemKeyInput(keynum, 0, 1);

            // bulkload
            int level = the_treep->bulkload (keynum, input, bfill);

            // randomize the leaf nodes
	    the_treep->randomize();

            // delete half of the keys
	    {int range= floor(keynum, worker_thread_num);
             if (range % 2 ==1) range=range-1;  // ensure range is even number

             std::thread threads[worker_thread_num];
	     for (int t=0; t<worker_thread_num; t++){
		threads[t] = std::thread ( [=](){
                     worker_id= t;
                     int start= range*t;
                     int end= ((t < worker_thread_num-1) ? start+range: keynum);
                     for (int ii=start; ii<end; ii+=2) {
	                key_type kk= input->keys[ii];
	                the_treep->del (kk);
		     }
		});
	     }
	     for (int t=0; t<worker_thread_num; t++) threads[t].join();
            }

            key_type start, end;
            the_treep->check (&start, &end);

            // duplicate deletions: do the same deletions again
	    {int range= floor(keynum, worker_thread_num);
             if (range % 2 ==1) range=range-1;  // ensure range is even number

             std::thread threads[worker_thread_num];
	     for (int t=0; t<worker_thread_num; t++){
		threads[t] = std::thread ( [=](){
                     worker_id= t;
                     int start= range*t;
                     int end= ((t < worker_thread_num-1) ? start+range: keynum);
                     for (int ii=start; ii<end; ii+=2) {
	                key_type kk= input->keys[ii];
	                the_treep->del (kk);
		     }
		});
	     }
	     for (int t=0; t<worker_thread_num; t++) threads[t].join();
            }

            start, end;
            the_treep->check (&start, &end);

	    // delete almost all the keys 
            // from right
            int skey;
	    int ekey = ((keynum-1) | 0x1);  // odd keys
            int step = keynum/8;
	    for (skey=keynum*3/4; skey>=keynum/2+2; skey -= step, step=(step>2?(step/2):2)) {

               // delete ekey, ekey-2, ekey-4, ... > skey
	       {int range= floor(ekey-skey, worker_thread_num);
                if (range % 2 ==1) range=range-1;  // ensure range is even number

                std::thread threads[worker_thread_num];
                for (int t=0; t<worker_thread_num; t++){
                   threads[t] = std::thread ( [=](){
                        worker_id= t;
                        int end= ekey - range*t;
                        int start= ((t < worker_thread_num-1) ? ekey-range: skey);
                        for (int ii=end; ii>start; ii-=2) {
                           key_type kk= input->keys[ii];
                           the_treep->del (kk);
                        }
                   });
                }
                for (int t=0; t<worker_thread_num; t++) threads[t].join();
               }

               ekey= ((skey & 0x1) ? skey : skey-1);
               
               the_treep->check (&start, &end);
	       assert ((start<=input->keys[1]) && (end >= input->keys[ekey]));
	    }

            // from left 
            step = keynum/8;
            skey = 1;
	    for (ekey=keynum/4; ekey<=keynum/2-2; ekey += step, step=(step>2?(step/2):2)) {
               // delete skey, skey+2, skey+4, ... , <ekey
	       {int range= floor(ekey-skey, worker_thread_num);
                if (range % 2 ==1) range=range-1;  // ensure range is even number

                std::thread threads[worker_thread_num];
                for (int t=0; t<worker_thread_num; t++){
                   threads[t] = std::thread ( [=](){
                        worker_id= t;
                        int start= skey + range*t;
                        int end= ((t < worker_thread_num-1) ? start+range: ekey);
                        for (int ii=start; ii<end; ii+=2) {
                           key_type kk= input->keys[ii];
                           the_treep->del (kk);
                        }
                   });
                }
                for (int t=0; t<worker_thread_num; t++) threads[t].join();
               }

               skey= ((ekey & 0x1) ? ekey : ekey+1);

               the_treep->check (&start, &end);
	       assert (start<=input->keys[skey]);
	    }

            printf ("delete is good!\n");
          }

          // *****************************************************************
          // Test Preparation
          // *****************************************************************

          // ---
          // bulkload <key_num> <key_file> <fill_factor>
          // ---
          else if (strcmp (argv[0], "bulkload") == 0) {
            // get params
            if (argc < 4) usage (cmd);
            int keynum = atoi (argv[1]);
	    char *keyfile = argv[2];
            float bfill; sscanf (argv[3], "%f", &bfill);
            argc -= 4; argv += 4;

	    printf ("-- bulkload %d %s %f\n", keynum, keyfile, bfill);

	    // Input
	    bufferedKeyInput *input = new bufferedKeyInput(keyfile, 0, keynum);

            // bulkload then check
            int level = the_treep->bulkload (keynum, input, bfill);
            printf ("root is at %d level\n", level);

            key_type start, end;
            the_treep->check (&start, &end);

            // free keys
            delete input;
          }

	  // ---
	  // randomize
	  // ---
	  else if (strcmp (argv[0], "randomize") == 0) {
            argc -= 1; argv += 1;

	    printf ("-- randomize\n");
	    the_treep->randomize();

            key_type start, end;
            the_treep->check (&start, &end);
	  }

          // ---
          // stable <key_num> <key_file>
          // ---
          else if (strcmp (argv[0], "stable") == 0) {
            // get params
            if (argc < 3) usage (cmd);
            int keynum = atoi (argv[1]);
            char *keyfile = argv[2];
            argc -= 3; argv += 3;

            printf ("-- stable %d %s\n", keynum, keyfile);

            // Input
            bufferedKeyInput *input = new bufferedKeyInput(keyfile, 0, keynum);

	    // the keyfile is specially prepared for stable operation
	    // the first 1/10 of the file is sorted
            // the rest of 9/10 is random

            // bulkload 10% of the keys
	    int bulkload_num = keynum/10;
            int level = the_treep->bulkload (bulkload_num, input, 1.0);
	    printf ("After bulkloading %d keys, level is %d\n",
		     bulkload_num, level);

            // insertion: [bulkload_num, keynum-1], keynum-bulkload_num keys
            int range= floor(keynum-bulkload_num, worker_thread_num);

            // run tests with multiple threads
            std::thread threads[worker_thread_num];
            for (int t=0; t<worker_thread_num; t++){
                threads[t] = std::thread ( [=](){
                     worker_id= t;
                     int start= bulkload_num + range*t;
                     int end= ((t<worker_thread_num-1)? start+range: keynum);
                     
                     keyInput *cursor= input->openCursor(start, end-start);
                     for (int ii=start; ii<end; ii++) {
                        key_type kk= cursor->get_key(ii);
                        the_treep->insert (kk, (void *) kk);
                     }
                     input->closeCursor(cursor);
                });
            }
            for (int t=0; t<worker_thread_num; t++) threads[t].join();

            key_type start, end;
            the_treep->check (&start, &end);

            printf ("root is at %d level\n", the_treep->level());

            // free keys
            delete input;
          }


          // *****************************************************************
          // Performance Tests
          // *****************************************************************

          // ---
          // lookup <key_num> <key_file>
          // ---
          else if (strcmp (argv[0], "lookup") == 0) {
            // get params
            if (argc < 3) usage (cmd);
            int keynum = atoi (argv[1]);
            char *keyfile = argv[2];
            argc -= 3; argv += 3;

            printf ("-- lookup %d %s\n", keynum, keyfile);

            // load keys from the file into an array in memory
            Int64 * key = getKeys (keyfile, keynum);

            // test
            unsigned long long total_us= 0;

	    std::thread threads[worker_thread_num];
            int range= floor(keynum, worker_thread_num);
            std::atomic<int> found;
            found= 0;

            clear_cache ();

            TEST_PERFORMANCE(total_us, do {
                if (worker_thread_num > 1) {
                  for (int t=0; t<worker_thread_num; t++) {
                      threads[t] = std::thread ( [=, &found](){
                           worker_id= t;
                           int start= range*t;
                           int end= ((t < worker_thread_num-1) ? start+range: keynum);
                           int th_found= lookupTest(key, start, end);
                           if (debug_test) found.fetch_add(th_found);
                      });
                  }
                  for (int t=0; t<worker_thread_num; t++) threads[t].join();
                } // end worker_thread_num > 1

                // worker_thread_num == 1
                else {
                  found= lookupTest(key, 0, keynum);
                }
            }while(0))

	    if (debug_test) {
	      printf ("lookup is good!\n");
              printf("found %d keys\n", found.load());
	    }

            free (key);
          }

          // ---
          // insert <key_num> <key_file>
          // ---
          else if (strcmp (argv[0], "insert") == 0) {
            // get params
            if (argc < 3) usage (cmd);
            int keynum = atoi (argv[1]);
            char *keyfile = argv[2];
            argc -= 3; argv += 3;

            printf ("-- insert %d %s\n", keynum, keyfile);

            // get keys from the file
            Int64 * key = getKeys (keyfile, keynum);

            // test
            unsigned long long total_us= 0;

            std::thread threads[worker_thread_num];
            int range= floor(keynum, worker_thread_num);
            std::atomic<int> found;
            found= 0;

            clear_cache ();

#ifdef NVMFLUSH_STAT
	    NVMFLUSH_STAT_init();
#endif

            TEST_PERFORMANCE(total_us, do {
                if (worker_thread_num > 1) {
                  for (int t=0; t<worker_thread_num; t++) {
                      threads[t] = std::thread ( [=, &found](){
                           worker_id= t;
                           int start= range*t;
                           int end= ((t < worker_thread_num-1) ? start+range: keynum);
                           int th_found= insertTest(key, start, end);
                           if (debug_test) found.fetch_add(th_found);
                      });
                  }
                  for (int t=0; t<worker_thread_num; t++) threads[t].join();
                } // end worker_thread_num > 1

                // worker_thread_num == 1
                else {
                  found= insertTest(key, 0, keynum);
                }
            }while(0))

#ifdef NVMFLUSH_STAT
	    NVMFLUSH_STAT_print();
#endif

            if (debug_test) {

              printf ("Insert %d keys / %d keys\n", found.load(), keynum);

              key_type start, end;
              the_treep->check (&start, &end);

              if (found.load() == keynum) {
                  printf ("Insertion is good!\n");
              }
              else {
                  printf ("%d keys are not successfully inserted!\n", keynum - found.load());
              }
            }

            free (key);
          }

          // ---
          // del <key_num> <key_file>
          // ---
          else if (strcmp (argv[0], "del") == 0) {
            // get params
            if (argc < 3) usage (cmd);
            int keynum = atoi (argv[1]);
            char *keyfile = argv[2];
            argc -= 3; argv += 3;

            printf ("-- del %d %s\n", keynum, keyfile);

            // get keys from the file
            Int64 * key = getKeys (keyfile, keynum);

            // test
            unsigned long long total_us= 0;

            std::thread threads[worker_thread_num];
            int range= floor(keynum, worker_thread_num);
            std::atomic<int> found;
            found= 0;

            clear_cache ();

#ifdef NVMFLUSH_STAT
            NVMFLUSH_STAT_init();
#endif

            TEST_PERFORMANCE(total_us, do {
                if (worker_thread_num > 1) {
                  for (int t=0; t<worker_thread_num; t++) {
                      threads[t] = std::thread ( [=, &found](){
                           worker_id= t;
                           int start= range*t;
                           int end= ((t < worker_thread_num-1) ? start+range: keynum);
                           int th_found= delTest(key, start, end);
                           if (debug_test) found.fetch_add(th_found);
                      });
                  }
                  for (int t=0; t<worker_thread_num; t++) threads[t].join();
                } // end worker_thread_num > 1

                // worker_thread_num == 1
                else {
                  found= delTest(key, 0, keynum);
                }
            }while(0))

#ifdef NVMFLUSH_STAT
            NVMFLUSH_STAT_print();
#endif

            if (debug_test) {

              key_type start, end;
              the_treep->check (&start, &end);

              if (found.load() == 0) {
                  printf ("Deletion is good!\n");
              }
              else {
                  printf ("%d keys are not successfully deleted!\n", found.load());
              }
            }

            free (key);
          }

	  else {
	    fprintf (stderr, "Unknown command: %s\n", argv[0]);
	    usage (cmd);
	  }
	} // end of while

	return 0;
}

