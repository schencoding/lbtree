/* File Name: getinsert.c
 * Author:    Shimin Chen
 *
 * Description: generate insertion keys
 *              
 *              getinsert <key_num> <input_keyfile> <insert_num> <output_file>
 *
 *              randomly generate "insert_num" distinct output keys that are 
 *              not in the set of input keys.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>

#include "keygen.h"

Int64 * get_keys (char *filename, Int64 num)
{int fd;
 Int64 *p;

   fd = open (filename, 0, 0600);
   if (fd == -1) {
     perror ("open"); exit(1);
   }

   p = (Int64 *) malloc (num * sizeof(Int64));
   if (p == NULL) {
     printf ("malloc error\n"); exit (1);
   } 

   if (read (fd, p, num*sizeof(Int64)) != num * sizeof(Int64))
     {perror ("read"); exit (1);}

   close (fd);
   return p;
}

static void write_once (char *name, void *buf, Int64 nbyte)
{int fd;

    fd = creat (name, 0644);
    if (fd == -1) {
      perror (name); exit (1);
    }

    if (write (fd, buf, nbyte) != nbyte) {
      perror ("write"); exit (1);
    }

    close (fd);
}

int compare (const void * ip1, const void * ip2)
{
        Int64 tt= (*(Int64 *)ip1 - *(Int64 *)ip2);
        return ((tt>0)?1: ((tt<0)?-1:0));
}

static inline Int64 gen_a_key()
{       
           Int64 a= random();
           Int64 b= random();
           Int64 c= random();
           return (((a<<32)^(b<<16)^(c)) & (0x7FFFFFFFFFFFFFFFLL));
}

Int64 get_not_in_lower (Int64 keys[], Int64 keynum, Int64 threashold)
{Int64 k;

   do{
     k = gen_a_key();
     k = k % threashold;
   }while(bsearch (&k, keys, keynum ,sizeof(Int64), compare)!=NULL);

   return k;
}

Int64 get_not_in_higher (Int64 keys[], Int64 keynum, Int64 threashold)
{Int64 k;

   do{
     k = gen_a_key();
   } while((k < threashold) || 
      (bsearch (&k, keys, keynum ,sizeof(Int64), compare)!=NULL));

   return k;
}

int is_in (Int64 keys[], Int64 keynum, Int64 k)
{Int64 i;
   for (i=0; i<keynum; i++)
      if (k == keys[i])
        return 1;
   return 0;
}

void sortkey (Int64 keynum, Int64 key[])
{Int64 i, count;
      do {
        qsort (key, keynum, sizeof(Int64), compare);
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

void shuffle (Int64 from[], Int64 to[], Int64 keynum)
{Int64 r, i, j;

        j=0;
        srand48(time(NULL));
        for (i=keynum-1; i>=0; i--) {
           // r is uniformly distributed in 0,1,...,i
           r = drand48()*(i+1);
           to[j++] = from[r];
           from[r] = from[i];
        }
}

int main (int argc, char *argv[])
{
 Int64 keynum;
 char *input_keyfile;
 Int64 insertnum;
 char *output_file;

 Int64 *keys;
 Int64 *insertion;
 Int64 i, count;

 Int64 threshold, insertnum_first_80, count_lower;

     if (argc < 5) {
       fprintf (stderr, "Usage: %s <key_num> <input_keyfile> <insert_num> <output_file>\n", argv[0]);
       exit (1);
     }
     keynum = atoll (argv[1]);
     input_keyfile = argv[2];
     insertnum = atoll (argv[3]);
     output_file = argv[4];

     /* get input keys */
     keys = get_keys (input_keyfile, keynum);
     threshold= keys[(int)(keynum / 5)];

     /* allocate memory */
     insertion = (Int64 *) malloc (insertnum * sizeof(Int64));
     if (insertion == NULL) {
       printf ("no memory\n"); exit (1);
     }

     // initial insertion keys
     insertnum_first_80 = insertnum / 5 * 4;
     printf ("insertnum_first_80 = %lld\n", insertnum_first_80);

     for (i=0; i<insertnum_first_80; i++) {
          insertion[i] = get_not_in_lower (keys, keynum, threshold);
     }
     for (i=insertnum_first_80; i<insertnum; i++) {
          insertion[i] = get_not_in_higher (keys, keynum, threshold);
     }

      // remove duplicates
      do {
        qsort (insertion, insertnum, sizeof(Int64), compare);
        count = 0;
        for (i=0; i<insertnum - 1; i++)
           if (insertion[i] == insertion[i+1]) {
             count ++; 
             if (insertion[i] < threshold)
                 insertion[i] = get_not_in_lower (keys, keynum, threshold);
             else
                 insertion[i] = get_not_in_higher (keys, keynum, threshold);
           }
        if (count > 0)
          printf ("%lld duplicates found\n", count);
        } while (count > 0);

       // shuffle the insertion keys
     shuffle(insertion, keys, insertnum);

     write_once (output_file, keys, insertnum*sizeof(Int64));

     count_lower= 0;
     for (i=0; i<insertnum; i++)
         count_lower += (keys[i] < threshold);

     printf("%lld keys are lower than %lld\n", count_lower, threshold);

     /* free memory */
     free (insertion); free (keys);

     return 0;
}
