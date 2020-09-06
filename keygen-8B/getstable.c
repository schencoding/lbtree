/* File Name: getstable.c
 * Author:    Shimin Chen
 *
 * Description: generate keys for generating mature trees
 *              
 *             getstable <input_keynum> <input_keyfile> <output_file>
 *
 *             The output_file contains the same set of keys in input_keyfile
 *             but in different order.  The first 10% of output keys are
 *             randomly selected from input keys.  Then they are sorted.
 *             The rest 10% of output keys are the rest of the input keys
 *             in random order.
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

int compare (const void * ip1, const void * ip2)
{
	Int64 tt= (*(Int64 *)ip1 - *(Int64 *)ip2);
	return ((tt>0)?1: ((tt<0)?-1:0));
}

int main (int argc, char *argv[])
{
 Int64 keynum;
 char *input_keyfile, *output_file;
 Int64 *keys;
 Int64 *stable;
 Int64 i, n;

     /* get params */
     if (argc < 4) {
       fprintf (stderr, "Usage: %s <input_keynum> <input_keyfile> <output_file>\n", argv[0]);
       exit (1);
     }
     keynum = atoll(argv[1]);
     input_keyfile = argv[2];
     output_file = argv[3];
     
     /* read keys and allocate memory */
     keys = get_keys (input_keyfile, keynum);
     stable = (Int64 *) malloc (keynum * sizeof(Int64));
     if (stable == NULL) {
       printf ("no memory\n"); exit (1);
     }

     /* get random keys */
     shuffle (keys, stable, keynum);

     /* sort the first keynum/10  keys */
     n = keynum/10;
     qsort (stable, n, sizeof(Int64), compare);
     for (i=0; i<n-1; i++)
        if (stable[i] >= stable[i+1]) {
          printf ("%lld error\n", i);
	  exit (1);
	}

     write_once (output_file, stable, keynum*sizeof(Int64));

     /* free memory */
     free (stable); free (keys);

     return 0;
}
