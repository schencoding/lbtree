/* File Name: getdelete.c
 * Author:    Shimin Chen
 *
 * Description: generate keys for deletion experiments
 *              
 *             getdelete <key_num> <input_keyfile> <delete_num> <output_file>
 *
 *             generate "delete_num" of distinct keys that exist in the
 *             input_keyfile.
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

// try to reuse the code of getstable.c as much as possible
int main (int argc, char *argv[])
{
 Int64 keynum;
 char *input_keyfile;
 Int64 deletenum;
 char *output_file;

 Int64 *keys;
 Int64 *deletion;

     if (argc < 5) {
       fprintf (stderr, "Usage: %s <key_num> <input_keyfile> <delete_num> <output_file>\n", argv[0]);
       exit (1);
     }
     keynum = atoll (argv[1]);
     input_keyfile = argv[2];
     deletenum = atoll (argv[3]);
     output_file = argv[4];

     keys = get_keys (input_keyfile, keynum);
     deletion = (Int64 *) malloc (keynum * sizeof(Int64));
     if (deletion == NULL) {
       printf ("no memory\n"); exit (1);
     }

     shuffle (keys, deletion, keynum);

     write_once (output_file, deletion, deletenum*sizeof(Int64));

     // free memory
     free (deletion); free (keys);

     return 0;
}
