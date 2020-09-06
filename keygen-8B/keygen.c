/* File Name: keygen.c
 * Author:    Shimin Chen
 *
 * Description: generate keys
 *              
 *             - random keys (may have duplicates)
 *             - sorted random keys (no duplicates)
 *             - natural numbers
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>

#include "keygen.h"

/* Generating keynum random numbers */

static inline Int64 gen_a_key()
{
	   Int64 a= random();
	   Int64 b= random();
	   Int64 c= random();
	   return (((a<<32)^(b<<16)^(c)) & (0x7FFFFFFFFFFFFFFFLL));
}

void keygen (Int64 keynum, Int64 key[])
{Int64 i;
	srandom (time(NULL));
	for (i=0; i<keynum; i++) {
	   key[i] =  gen_a_key();
	}

	/* random () returns positive integers */
}

/* Sorting the keys */

int compare (const void * ip1, const void * ip2)
{
	Int64 tt= (*(Int64 *)ip1 - *(Int64 *)ip2);
	return ((tt>0)?1: ((tt<0)?-1:0));
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

/* main */

int main (int argc, char *argv[])
{Int64 keynum;
 char cmd;
 char *filename;

 int fd;
 Int64 *key, i;

  /* input param */
  if (argc < 4) {
    fprintf (stderr, "Usage: %s <key_num> <sort|random|natural> <filename>\n", argv[0]);
    exit (1);
  }
  keynum = atoll (argv[1]);
  cmd = argv[2][0];
  filename = argv[3];

  /* allocate memory */
  key = (Int64 *)malloc (sizeof(Int64) * keynum);
  if (key == NULL) {
    printf ("no memory\n"); exit (1);
  }
  
  /* generate keys */
  if (cmd == 'n') {
    // generate natural numbers
    for (i=0; i<keynum; i++)
       key[i] = i+1;
  }
  else {
    printf ("generating %lld random keys ...\n", keynum);
    keygen (keynum, key);
    if (cmd == 's') {
      printf ("getting sorted keys ...\n");
      sortkey (keynum, key);
    }
  }

  /* output */
  printf ("writing keys into %s\n", filename);
  fd = creat (filename, 0644);
  if (fd == -1) {
    perror ("creat"); exit (1);
  }
  if (write(fd, key, sizeof(Int64)*keynum) != sizeof(Int64)*keynum) {
    fprintf (stderr, "write error\n"); exit (1);
  }
  close (fd);
  
  /* free */
  free (key);

  return 0;
}
