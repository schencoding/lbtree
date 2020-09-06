/* File Name: statkey.c
 * Author:    Shimin Chen
 *
 * Description: compute histogram for every 8-bit
 *              
 *             statkey <key_num> <input_keyfile>
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

// collect stats for [shift, shift+8) bits
void get_stats (Int64 keynum, Int64 keys[], int shift)
{
	Int64 count[256];
	int ii;
	Int64 i;

	for(ii=0; ii<256; ii++) count[ii]= 0;

	for (i=0; i<keynum; i++) {
		int val= ((keys[i] >> shift) & 0x00ff);
		count[val] ++;
	}

	printf("shifting %d bits:\n", shift);
	printf("----------------------------------------------------------------------\n");
	for(ii=0; ii<256; ii++) {
		printf("count[%02x]= %7.2lf%%\n", ii, (double)count[ii]/(double)keynum*100.0);
	}
	printf("\n\n");
	fflush(stdout);
}

// try to reuse the code of getstable.c as much as possible
int main (int argc, char *argv[])
{
 Int64 keynum;
 char *input_keyfile;
 Int64 *keys;
 int shift;

     if (argc < 3) {
       fprintf (stderr, "Usage: %s <key_num> <input_keyfile>\n", argv[0]);
       exit (1);
     }
     keynum = atoll (argv[1]);
     input_keyfile = argv[2];

     keys = get_keys (input_keyfile, keynum);

     for(shift=0; shift<64; shift+=8) {
	get_stats(keynum, keys, shift);
     }

     free (keys);

     return 0;
}
