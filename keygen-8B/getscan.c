/* File Name: getscan.c
 * Author:    Shimin Chen
 *
 * Description: generate keys for scan
 *              
 *             getscan <input_keynum> <input_keyfile> <num_keys> <distance> <output_file>
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

void getScanKeys (Int64 keynum, Int64 from[], Int64 num_scan_keys, Int64 to[], Int64 distance)
{Int64 r, i;

	srand48(time(NULL));
	for (i=0; i<num_scan_keys; i++) {
	   // r is uniformly distributed in 0,1,...,i
	   r = drand48()*(keynum-distance);
	   to[i] = from[r];
        }
}

int main (int argc, char *argv[])
{
 Int64 keynum;
 char *input_keyfile, *output_file;
 Int64 *keys;
 Int64 *scan;
 Int64 distance, num_scan_keys;

     /* get params */
     if (argc < 6) {
       fprintf (stderr, "Usage: %s <input_keynum> <input_keyfile> <num_scan_keys> <distance> <output_file>\n", argv[0]);
       exit (1);
     }
     keynum = atoll(argv[1]);
     input_keyfile = argv[2];
     num_scan_keys= atoll(argv[3]);
     distance= atoll(argv[4]);
     output_file = argv[5];
     
     /* read keys and allocate memory */
     keys = get_keys (input_keyfile, keynum);
     scan = (Int64 *) malloc (num_scan_keys * sizeof(Int64));
     if (scan == NULL) {
       printf ("no memory\n"); exit (1);
     }

     /* get random keys */
     getScanKeys (keynum, keys, num_scan_keys, scan, distance);

     write_once (output_file, scan, num_scan_keys*sizeof(Int64));

     /* free memory */
     free (scan); free (keys);

     return 0;
}
