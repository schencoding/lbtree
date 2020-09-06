/**
 * @file keyinput.h
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 * 
 * TBD
 *   
 * @section DESCRIPTION
 * 
 * keyInput provides the mechanisms to read index keys from files.
 */

#ifndef _BTREE_KEYINPUT_H
#define _BTREE_KEYINPUT_H
/* ---------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <malloc.h>

/* ---------------------------------------------------------------------- */

typedef long long Int64;

/**
 * keyInput class defines the get_key virtual method.
 */
class keyInput {
 public:
  /**
   * get the index-th key
   * 
   * @param index  the index-th key
   * @return       the key
   */
   virtual Int64 get_key (Int64 index)
   {
	fprintf (stderr, "get_key is not implemented!\n");
	exit (1);
   }

  /**
   * each thread can open cursor to read from a different location
   * if the keys are already in an array, then do nothing
   */
   virtual keyInput *openCursor(Int64 start_key, Int64 keynum) {return this;}

   virtual void closeCursor(keyInput * cursor) { }

}; // keyInput


/**
 * bufferedKeyInput class gradually reads keys from a file using a memory buffer
 */
class bufferedKeyInput: public keyInput {
 private:
   const char *key_file;
   int key_fd; 
   Int64 key_num;    // total number of keys
   Int64 key_bottom; // absolute index of key_buffer[0]
   Int64 key_top;    // absolute index beyond keys in buffer
   Int64 key_start;

#define KEY_BUFFER_SIZE   (1024*1024)
   Int64 *key_buffer;

 public:
  /**
   * constructor
   *
   * @param filename  the file to read keys from
   * @param start_key read from start_key from the file
   * @param keynum    the number of keys to read
   */
   bufferedKeyInput (const char *filename, Int64 start_key, Int64 keynum)
   {
     key_file= filename;
     key_fd = open (filename, 0, 0600);
     if (key_fd < 0) {
       perror (filename); exit (1);
     }

     Int64 myoff= start_key*sizeof(Int64);
     if (lseek(key_fd, myoff, SEEK_SET) != myoff) {
        fprintf(stderr, "can't seek to start_key=%lld off=%lld\n", 
                         start_key, myoff);
        perror("lseek"); exit(1);
     }

     key_num = keynum;
     key_bottom = 0;
     key_top = 0;
     key_start= start_key;

     key_buffer= (Int64 *) memalign(4096, KEY_BUFFER_SIZE);
     if (!key_buffer) {perror("memalign"); exit(1);}
   }

   ~bufferedKeyInput ()
   {
     free (key_buffer);
     close (key_fd);
   }

   // index should be non-descending
   // index starts at start_key
   Int64 get_key (Int64 index)
   {int len;

     index -= key_start;

     while (index >= key_top) {
         len = read (key_fd, key_buffer, KEY_BUFFER_SIZE);
         if (len < 0) { perror("read"); exit(1); }
         key_bottom = key_top;
         key_top += len/sizeof(Int64);
     }

     Int64 pos = index - key_bottom;
     assert (pos >= 0);
     return key_buffer[pos];
   }

   keyInput *openCursor(Int64 start_key, Int64 keynum) 
   {
     return new bufferedKeyInput(key_file, start_key, keynum);
   }

   void closeCursor(keyInput * cursor) 
   { 
     bufferedKeyInput *p= (bufferedKeyInput *)cursor;
     delete p;
   }

}; // bufferedKeyInput

/**
 * simpleKeyInput class generates natural numbers as keys
 */
class simpleKeyInput: public keyInput {
 private:
   Int64 key_num;
   Int64 key_start;
   Int64 key_step;

 public:
  /**
   * constructor
   *
   * @param num    number of keys to generate
   * @param start  the start index of the keys
   * @param step   number of keys to skip per get_key
   */
   simpleKeyInput (Int64 num, Int64 start, Int64 step)
   {
	key_num = num;
	key_start = start;
	key_step = step;
   }

   Int64 get_key (Int64 index)
   {
	return (key_start + key_step * index);
   }
};

/**
 * inMemKeyInput class generates random keys
 */
class inMemKeyInput: public keyInput {
 public:
	Int64	key_num;
	Int64	*keys;
	Int64	key_start;
	Int64	key_step;

 private:
        static inline Int64 gen_a_key()
        {
                Int64 a= random();
                Int64 b= random();
                Int64 c= random();
                return (((a<<32)^(b<<16)^(c)) & (0x7FFFFFFFFFFFFFFFLL));
        }

#define KEYS_FOR_DBG

        static void keygen (Int64 keynum, Int64 key[])
        {Int64 i;
                srandom (time(NULL));
                for (i=0; i<keynum; i++) {
#ifdef KEYS_FOR_DBG
                   key[i] =  i+1;
#else
                   key[i] =  gen_a_key();
#endif
                }
        
                /* random () returns positive integers */
        }
        
        /* Sorting the keys */
        
        static int compare (const void * ip1, const void * ip2)
        {
                Int64 tt= (*(Int64 *)ip1 - *(Int64 *)ip2);
                return ((tt>0)?1: ((tt<0)?-1:0));
        }
        
        static void sortkey (Int64 keynum, Int64 key[])
        {Int64 i, count;
              do {
                qsort (key, keynum, sizeof(Int64), inMemKeyInput::compare);
                count = 0;
                for (i=0; i<keynum - 1; i++)
                   if (key[i] == key[i+1]) {
                     count ++;
                     key[i] = gen_a_key();
                   }
                if (count > 0)
                  printf ("%lld duplicates found\n", count);
                } while (count > 0);
        }


 public:
       /**
	* generate sorted random keys
        *
        * @param num    number of keys to generate
        * @param start  the start index of the keys
        * @param step   number of keys to skip per get_key
        */
	inMemKeyInput (Int64 num, Int64 start, Int64 step)
	{
		key_num= num;
		keys= new Int64[num];
		assert(keys);

		keygen(key_num, keys);
		sortkey(key_num, keys);

		key_start= start;
		key_step= step;
	}

	~inMemKeyInput()
	{
		delete[] keys;
	}

        Int64 get_key (Int64 index)
        {
	        Int64 ii= key_start + key_step*index;
                assert((0<=ii) && (ii<key_num));
                return keys[ii];
        }
}; // inMemKeyInput

/* ---------------------------------------------------------------------- */
#endif /* _BTREE_KEYINPUT_H */
