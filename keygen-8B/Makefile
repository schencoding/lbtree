CC     = gcc
CFLAGS = -O2 -g -D_FILE_OFFSET_BITS=64 -Wall

TARGETS = keygen showkey getstable getinsert getdelete statkey getscan get20-80Insert
# --------------------------------------------------------------------------
all: ${TARGETS}

keygen: keygen.c
showkey: showkey.c

getstable: getstable.c
getinsert: getinsert.c
get20-80Insert: get20-80Insert.c
getdelete: getdelete.c
getstart:  getstart.c
getscan: getscan.c

statkey: statkey.c

clean:
	-rm -f ${TARGETS} a.out *.o core
