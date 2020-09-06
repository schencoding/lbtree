/**
 * @file nvmlog-common.h
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 * 
 * TBD
 *       
 * @section DESCRIPTION
 *
 * This header defines constants and useful macros for NVM
 */

#ifndef _NVM_COMMON_H
#define _NVM_COMMON_H

/* -------------------------------------------------------------- */
#include "mempool.h"
#include "nodepref.h"

/* -------------------------------------------------------------- */
// must define one of the following:
// NVMFLUSH_REAL:  flush
// NVMFLUSH_STAT:  no flush, count stats
// NVMFLUSH_DUMMY: no flush

#define NVMFLUSH_REAL

#if   defined(NVMFLUSH_REAL)
#undef NVMFLUSH_DUMMY
#undef NVMFLUSH_STAT

#elif defined(NVMFLUSH_STAT)
#undef NVMFLUSH_DUMMY
#undef NVMFLUSH_REAL

#elif defined(NVMFLUSH_DUMMY)
#undef NVMFLUSH_STAT
#undef NVMFLUSH_REAL

#else
#error "must define NVMFLUSH_DUMMY, or NVMFLUSH_STAT, or NVMFLUSH_REAL"
#endif


/* -------------------------------------------------------------- */
#if   defined(NVMFLUSH_REAL)
/* -------------------------------------------------------------- */
// use clwb and sfence

/**
 * flush a cache line
 *
 * @param addr   the address of the cache line
 */
static inline
void clwb(void * addr)
{ asm volatile("clwb %0": :"m"(*((char *)addr))); } 

/**
 * flush [start, end]
 *
 * there are at most two lines.
 */
static inline
void clwb2(void *start, void *end)
{
  clwb(start);
  if (getline(start) != getline(end)) {
    clwb(end);
  }
}

/**
 * flush [start, end]
 *
 * there can be 1 to many lines
 */
static inline
void clwbmore(void *start, void *end)
{ 
  unsigned long long start_line= getline(start);
  unsigned long long end_line= getline(end);
  do {
    clwb((char *)start_line);
    start_line += CACHE_LINE_SIZE;
  } while (start_line <= end_line);
}

/**
 * call sfence
 */
static inline
void sfence(void)
{ asm volatile("sfence"); }

/* -------------------------------------------------------------- */
#elif defined(NVMFLUSH_STAT)
/* -------------------------------------------------------------- */

#define NVMFLUSH_STAT_DEFS   long long num_clwb, num_sfence

extern long long num_clwb, num_sfence;

static inline
void NVMFLUSH_STAT_init()
{
    num_clwb=0;
    num_sfence= 0;
}

static inline
void NVMFLUSH_STAT_print()
{
    printf("num_clwb=%lld, num_sfence=%lld\n", num_clwb, num_sfence);
}

static inline
void clwb(void * addr)
{
   num_clwb ++;
 //printf("clwb(%p)\n", addr);
}

static inline
void clwb2(void *start, void *end)
{
  num_clwb ++;
  if (getline(start) != getline(end)) {
    num_clwb ++;
  }
  // printf("clwb2(%p, %p)\n", start, end);
}

static inline
void clwbmore(void *start, void *end)
{
  unsigned long long start_line= getline(start);
  unsigned long long end_line= getline(end);
  num_clwb += (end_line+CACHE_LINE_SIZE-start_line)/CACHE_LINE_SIZE;

  // printf("clwbmore(%p, %p)\n", start, end);
}

static inline
void sfence(void)
{
   num_sfence ++;
 //printf("sfence()\n");
}

/* -------------------------------------------------------------- */
#elif defined(NVMFLUSH_DUMMY)
/* -------------------------------------------------------------- */

static inline void clwb(void * addr) {}
static inline void clwb2(void *start, void *end) {}
static inline void clwbmore(void *start, void *end) {}
static inline void sfence(void) {}

#endif  // NVMFLUSH_DUMMY

/* -------------------------------------------------------------- */

#define LOOP_FLUSH(cmd, ptr, nline)               \
{char *_p= (char *)(ptr);                         \
 char *_pend= _p + (nline)*CACHE_LINE_SIZE;       \
 for (; _p<_pend; _p+=(CACHE_LINE_SIZE)) cmd(_p); \
}

/* -------------------------------------------------------------- */
/**
 * copy src line to dest line using movntdq
 *
 * Both src and dest are 64 byte lines and aligned
 *
 * @param dest  destination line
 * @param src   src line
 */
static inline
void writeLineMOVNT(char *dest, char *src)
{   
  asm volatile(
    "movdqa   %4, %%xmm0    \n\t"
    "movdqa   %5, %%xmm1    \n\t"
    "movdqa   %6, %%xmm2    \n\t"
    "movdqa   %7, %%xmm3    \n\t"
    "movntdq  %%xmm0, %0    \n\t"
    "movntdq  %%xmm1, %1    \n\t"
    "movntdq  %%xmm2, %2    \n\t"
    "movntdq  %%xmm3, %3    \n\t" 
    : "=m"(dest[ 0]), "=m"(dest[16]), "=m"(dest[32]), "=m"(dest[48])
    :  "m"(src[ 0]),   "m"(src[16]),   "m"(src[32]),   "m"(src[48])
    : "xmm0", "xmm1", "xmm2", "xmm3",
      "memory");
}

/* -------------------------------------------------------------- */
// Log is kept for every thread
#ifndef NVM_LOG_SIZE
#define NVM_LOG_SIZE	   (1*MB)
#endif

/* -------------------------------------------------------------- */
// Tags
#define NL_INVALID       0x0

#define NL_NEW1B         0x1
#define NL_NEW2B         0x2
#define NL_NEW4B         0x3
#define NL_NEW8B         0x4
#define NL_NEWVCHAR      0x5

#define NL_WRITE1B       0x6
#define NL_WRITE2B       0x7
#define NL_WRITE4B       0x8
#define NL_WRITE8B       0x9
#define NL_WRITEVCHAR    0xa

#define NL_REDO1B        0xb
#define NL_REDO2B        0xc
#define NL_REDO4B        0xd
#define NL_REDO8B        0xe
#define NL_REDOVCHAR     0xf

#define NL_ALLOCNODE     0x10
#define NL_DELNODE       0x11

#define NL_NEXT_CHUNK    0x20  // go to the next chunk

#define NL_COMMIT        0x80
#define NL_ABORT         0x81
#define NL_ONGOING       0x82

/* -------------------------------------------------------------- */
/**
 * NvmLog_Log class implements the in-memory NVM log.
 *
 * It consists of a persistent NVM log buffer and volatile book-keeping states.
 * The log buffer is aligned at cache line boundary and its size is a multiple
 * of cache line size.  Each line in the log buffer has the following
 * structure:
 *
 * +-----------+--------+------------+-----+--------------------------+
 * | flag byte | offset |log byte[2] | ... | log byte[CACHE_LINE_SIZE-1] |
 * +-----------+--------+------------+-----+--------------------------+
 *
 * The flag byte = 7-bit checksum | 1-bit version
 *
 * offset = the offset of the first record that starts in this line
 *
 * The checksum can be used to check if all the data in the line is available.
 * The version will be flipped whenever the log buffer is wrapped around.
 */
class NvmLog_Log {

  public:
    typedef struct nl_log_pointer_{
       unsigned char  version_;
       unsigned char  offset_;  // 0 means that this line does not have a new record
       char *nextline_ptr_;
       char *next_ptr_;
    } nl_log_pointer_t;

  private:
    // Pointing to log area on NVM
    char *   nl_log_area_;              // the log area
    char *   nl_log_area_end_;

    // used for log writing
    nl_log_pointer_t  nl_log_wr_;

  public:

    /**
     * Initialize NvmLog_Log structure and start a new log buffer
     */
    void initLog()
    {
      // 1. allocate log buffer from NVM
      nl_log_area_= (char *) nvmpool_alloc(NVM_LOG_SIZE);

      if (!isaligned_atline(nl_log_area_)) {
        fprintf(stderr, "NvmLog error: log area is not aligned!\n");
        exit(1);
      }

      nl_log_area_end_= nl_log_area_ + NVM_LOG_SIZE;

      // 2. clear the log buffer
      memset(nl_log_area_, 0, NVM_LOG_SIZE);
      clwbmore(nl_log_area_, nl_log_area_end_-1);

      // 4. initialize nl_log_wr_
      nl_log_wr_.version_= 0;
      nl_log_wr_.offset_= 0;
      nl_log_wr_.nextline_ptr_= nl_log_area_end_;
      nl_log_wr_.next_ptr_= nl_log_area_end_;

      // 5. sfence
      sfence();
    }

   /**
    * Reset log write pointers to the beginning of log buffer
    * and flip the version
    */
    void prepareLogforWriting()
    {
      nl_log_wr_.version_ ^= 0x80;
      nl_log_wr_.offset_= 0;

      // nextline_ptr_ is the next line beyond the current line
      // which is written using next_ptr_
      nl_log_wr_.nextline_ptr_= nl_log_area_ + CACHE_LINE_SIZE;

      // next_ptr_ is the next byte to write in the log buffer
      nl_log_wr_.next_ptr_= nl_log_area_ + 2;
    }

    // for debugging
    void printLog()
    {
        printf("nl_log_area_:\n");
        for (char *p= nl_log_area_; p<nl_log_area_end_; p+=16) {
           if ((int)(p-nl_log_area_) % CACHE_LINE_SIZE==0) {
              printf("%8d:", (int)(p-nl_log_area_));
           }
           else {
              printf("         ");
           }
           for (int ii=0; ii<16; ii++)
              printf(" %02x", (unsigned char)p[ii]);
           printf("\n");
        }
    }

    // for debugging
    void printLogWritePos()
    {
      printf("write version_:%02x, offset_:%3d, next_ptr_:%d, nextline_ptr_:%d\n",
             nl_log_wr_.version_,
             nl_log_wr_.offset_,
             (int)(nl_log_wr_.next_ptr_ - (nl_log_wr_.nextline_ptr_-CACHE_LINE_SIZE)),
             (int)(nl_log_wr_.nextline_ptr_ - nl_log_area_));
    }

    // for debugging
    void printLogReadPos(nl_log_pointer_t *pos)
    {
      printf("read version_:%02x, offset_:%3d, next_ptr_:%d, nextline_ptr_:%d\n",
             pos->version_,
             pos->offset_,
             (int)(pos->next_ptr_ - nl_log_area_),
             (int)(pos->nextline_ptr_ - nl_log_area_));
    }

    // for debugging
    unsigned long long getLogOffset(void *p)
    {
      return (unsigned long long)((char *)p - nl_log_area_);
    }

  private:
    char computeCheckSum(char *line)
    {
        unsigned long long *p= (unsigned long long *)line;
        unsigned long long v=
                       (p[0]&0xffffffffffffff00ULL)
                       + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7];
        v += v>>32;
        v += v>>16;
        v += v>>8;
        return (char)v;
    }

    void setByteOne(char *line, char version)
    {
        char checksum= computeCheckSum(line);
        line[0]= ((checksum&0x7f)|version);
    }

    int checkByteOne(char *line, char version)
    {
        char checksum= computeCheckSum(line);
        return (line[0]==((checksum&0x7f)|version));
    }

    void completeLine2Log()
    {
       char *line= nl_log_wr_.nextline_ptr_ - CACHE_LINE_SIZE;
       line[1]= nl_log_wr_.offset_;
       setByteOne(line, nl_log_wr_.version_);
       clwb(line);
       sfence();

       if (nl_log_wr_.nextline_ptr_ < nl_log_area_end_) {
          nl_log_wr_.offset_= 0;
          nl_log_wr_.next_ptr_= nl_log_wr_.nextline_ptr_+2;
          nl_log_wr_.nextline_ptr_ += CACHE_LINE_SIZE;
       }
       else {
          prepareLogforWriting();
       }
    }

  public:
   /**
    * write a log record
    *
    * @param rec  the starting address of the log record
    * @param len  number of bytes of the record
    */
    void writeLog(const char *rec, int len)
    {
       int l= nl_log_wr_.nextline_ptr_ - nl_log_wr_.next_ptr_;
       if (nl_log_wr_.offset_ == 0) {
          nl_log_wr_.offset_= CACHE_LINE_SIZE-l;
       }

       while (len >= l) {
          memcpy(nl_log_wr_.next_ptr_, rec, l);
          rec += l; len -= l;
          completeLine2Log();
          l= CACHE_LINE_SIZE-2;
       } // end of while

       if (len > 0) {
          memcpy(nl_log_wr_.next_ptr_, rec, len);
          nl_log_wr_.next_ptr_ += len;
       }
    }

   /**
    * Flush the current line to log
    */
    void flushLog(void)
    {
       char *line= nl_log_wr_.nextline_ptr_ - CACHE_LINE_SIZE;
       if (nl_log_wr_.next_ptr_-line > 2) {
          nl_log_wr_.next_ptr_[0]= NL_INVALID;
          line[1]= nl_log_wr_.offset_;
          setByteOne(line, nl_log_wr_.version_);
          clwb(line);
       }

       sfence();
    }

    /*
     * crash recovery code  (not implemented here, but is do-able)
     *
     * 0. byte 0 in every cache line is the checksum version byte.
     * 1. scan the log from beginning to end.  look for the crash point.
     *    Q: which line is the last valid line?
     *    A: look for bad checksum or where version changes
     *    Q: which records are valid?
     *    A: use offset byte to find the first valid record in a line
     *
     * 2. log truncation
     *
     *    when truncating the log, we must make sure that previous modified 
     *    data are all written back to memory.  One way is to do clwb. 
     *    Another way is to read a portion of memory > cache size, to make sure
     *    that the cache is flushed.
     *
     *    at the log truncation point, we need to remember the starting location
     *    of the last transaction.
     *
     * 3. so crash recovery should work as follows
     *
     *    (1) retrieve the transaction start position
     *    (2) read the log, note down all the transactions
     *        All the transactions except the last one will be complete (commit/abort).  
     *        This is because our log instance is private to a thread.
     *    (3) For commit transactions, perform redo checks.
     *        for abort transactions, perform undo checks.
     *        for the last transaction if it is incomplete, undo it.
     *
     *    the redo-xxx log records in a transaction are always contiguous in the log.
     *    we can easily identify this during the forward scan.
     *    We can skip such redo-xxx records when doing undo.
     */

    // reading log

   /**
    * Get a nl_log_pointer_t structure that reflects the log position
    *
    * Upon returning, the fields of the nl_log_pointer_t structure have the
    * the following meanings. version_: current version 0/0x80; nextline_ptr_:
    * the next line in the log to write;  next_ptr_: the next byte in the log
    * to write; offset_: the position of the first record in the current line.
    *
    * While the movnt implementation keeps a temporary buffer, the returned
    * nl_log_pointer_t structure should have the same meaning as the clwb
    * implementation so that callers have consistent understanding.
    *
    * @param pos returned position
    */
    void getLogCurPos(nl_log_pointer_t *pos)
    {
        pos->version_ = nl_log_wr_.version_;
        pos->offset_= nl_log_wr_.offset_;

        pos->next_ptr_ = nl_log_wr_.next_ptr_;
        pos->nextline_ptr_= nl_log_wr_.nextline_ptr_;
    }

    int isSameAsCurPos(nl_log_pointer_t *pos)
    {
        char * cur_next_ptr;
        cur_next_ptr = nl_log_wr_.next_ptr_;
        return (pos->next_ptr_ == cur_next_ptr);
    }

   /**
    * prepare for reading forward
    *
    * Call before the first record to read
    *   getLogCurPos(&pos); 
    * then
    *   ...... write log  
    * after the last record
    *   prepareForRead(&pos);
    * We will read the log from pos->next_ptr_ to the current position.
    *
    * @param pos  the returned position from getLogCurPos
    */
    void prepareForRead(nl_log_pointer_t *pos) 
    {
        flushLog();
    }

   /**
    * obtain the end position of the log
    */
    char *getLogWriteEndPtr(void)
    {
        return nl_log_wr_.next_ptr_;
    }
    
   /**
    * read at most len bytes from the log into buf
    * 
    * This method will check the version and checksum, so it is more
    * efficient to read at least an entire line at a time.
    *
    * @param pos  log reading position
    * @param buf  buffer to hold the log data
    * @param len  buffer size in bytes
    * @retval  >0  number of bytes read into buf
    * @retval  0   reach the end of the log
    */
    int readLog(nl_log_pointer_t *pos, char *buf, int len)
    {
       char *buf0= buf;
       int l= pos->nextline_ptr_ - pos->next_ptr_;
       while ((len > 0)
           && checkByteOne(pos->nextline_ptr_-CACHE_LINE_SIZE, pos->version_)) {

          if (len < l) l= len;
          memcpy(buf, pos->next_ptr_, l);
          buf += l; len-= l; pos->next_ptr_ += l;

          if (pos->next_ptr_ == pos->nextline_ptr_) {
              if (pos->nextline_ptr_ < nl_log_area_end_) {
                pos->next_ptr_+=2;
                pos->nextline_ptr_ += CACHE_LINE_SIZE;
              }
              else {
                pos->version_ ^= 0x80;
                pos->next_ptr_= nl_log_area_+2;
                pos->nextline_ptr_ = nl_log_area_ + CACHE_LINE_SIZE;
              }
              pos->offset_= pos->next_ptr_[-1];
              l= CACHE_LINE_SIZE-2;
          }
       }
       return (buf - buf0);
    }

   /**
    * skip at most len bytes from the log
    * 
    * This method will check the version and checksum.
    *
    * @param pos  log reading position
    * @param len  bytes to skip
    * @retval  >0  number of bytes skipped
    * @retval  0   reach the end of the log
    */
    int readLogSkip(nl_log_pointer_t *pos, int len)
    {
       int num= 0;
       int l= pos->nextline_ptr_ - pos->next_ptr_;
       while ((len > 0)
           && checkByteOne(pos->nextline_ptr_-CACHE_LINE_SIZE, pos->version_)) {

          if (len < l) l= len;
          num += l; len-= l; pos->next_ptr_ += l;

          if (pos->next_ptr_ == pos->nextline_ptr_) {
              if (pos->nextline_ptr_ < nl_log_area_end_) {
                pos->next_ptr_+=2;
                pos->nextline_ptr_ += CACHE_LINE_SIZE;
              }
              else {
                pos->version_ ^= 0x80;
                pos->next_ptr_= nl_log_area_+2;
                pos->nextline_ptr_ = nl_log_area_ + CACHE_LINE_SIZE;
              }
              pos->offset_= pos->next_ptr_[-1];
              l= CACHE_LINE_SIZE-2;
          }
       }
       return num;
    }

   /**
    * prepare for reading backward
    *
    * call after the last record to read 
    *    getLogCurPos(&pos); 
    *    prepareForRead(&pos);
    * We will read the log from pos->next_ptr_-1 backward
    *
    * @param pos  the returned position from getLogCurPos
    */
    void prepareForReverseRead(nl_log_pointer_t *pos)
    {
       flushLog();

       pos->nextline_ptr_= (char *)getline(pos->next_ptr_);
       pos->version_= (pos->nextline_ptr_[0]&0x80);
    }

   /**
    * read backward at most len bytes from the log into buf
    * 
    * This method will check the version and checksum, so it is more
    * efficient to read at least an entire line at a time.
    *
    * @param pos  log reading position
    * @param buf  buffer to hold the data, buf[len-num..len-1] are valid
    * @param len  buffer size in bytes
    * @retval  >0  number of bytes read into buf
    * @retval  0   reach the end of the log
    */
    int readLogReverse(nl_log_pointer_t *pos, char *buf, int len)
    {
       char *p= buf+len;
       char *p_end= p;
       int l= (pos->next_ptr_ - pos->nextline_ptr_)-2;

       while ((len > 0)
           && checkByteOne(pos->nextline_ptr_, pos->version_)) {

          if (len < l) l= len;
          p -= l; pos->next_ptr_ -= l; len-= l;
          memcpy(p, pos->next_ptr_, l);

          if (pos->next_ptr_-2 == pos->nextline_ptr_) {
              if (pos->nextline_ptr_ > nl_log_area_) {
                pos->next_ptr_= pos->nextline_ptr_;
                pos->nextline_ptr_ -= CACHE_LINE_SIZE;
              }
              else {
                pos->version_ ^= 0x80;
                pos->next_ptr_= nl_log_area_end_;
                pos->nextline_ptr_ = nl_log_area_end_ - CACHE_LINE_SIZE;
              }
              pos->offset_= pos->nextline_ptr_[1];
              l= CACHE_LINE_SIZE-2;
          }
       }
       return (p_end - p);
    }

   /** 
    * skip backward at most len bytes from the log
    * 
    * This method will check the version and checksum.
    *
    * @param pos  log reading position
    * @param len  bytes to skip 
    * @retval  >0  number of bytes skipped
    * @retval  0   reach the end of the log
    */ 
    int readLogReverseSkip(nl_log_pointer_t *pos, int len)
    {
       int num= 0;
       int l= (pos->next_ptr_ - pos->nextline_ptr_)-2;
          
       while ((len > 0)
           && checkByteOne(pos->nextline_ptr_, pos->version_)) {
          
          if (len < l) l= len;
          num += l; pos->next_ptr_ -= l; len-= l;
                
          if (pos->next_ptr_-2 == pos->nextline_ptr_) {
              if (pos->nextline_ptr_ > nl_log_area_) {
                pos->next_ptr_= pos->nextline_ptr_;
                pos->nextline_ptr_ -= CACHE_LINE_SIZE;
              } 
              else {
                pos->version_ ^= 0x80;
                pos->next_ptr_= nl_log_area_end_;
                pos->nextline_ptr_ = nl_log_area_end_ - CACHE_LINE_SIZE;
              }
              pos->offset_= pos->nextline_ptr_[1];
              l= CACHE_LINE_SIZE-2;
          }
       }
       return num;
    }

    char * reverseAdjustPtr(char *ptr)
    {
      if ((((unsigned long long)(ptr)) & (CACHE_LINE_SIZE-1)) <= 2) {
        // if pointing at byte 2 of a chunk, point to byte 0
        // because readLogReverse will automatically move a pointer
        // from byte 2 to byte 0.
        ptr -= 2;
        if (ptr == nl_log_area_) ptr = nl_log_area_end_;
      }
      return ptr;
    }

}; // NvmLog_Log

/* -------------------------------------------------------------- */
// class NvmLog
//
// Usage:
// (1) insert startMiniTransaction immediately after the start of transaction
//     in transaction processing code.
// (2) insert commitMiniTransaction/abortMiniTransaction immediately before the
//     commit/abort of a transaction.
// (3) For every write to persistent memory location in a transaction, use 
//     one of the defined methods.
// (4) writexx is for overwirting a memory location; newxx is for writing
//     a memory location previously not used.
//
// Log format:
// (1) Every record: 
//     a tag     (1B)
//     addr      (8B)
//     length    (4B) (only for xxxVCHAR)
//     old value      (only for WRITEXX)
//     new value
//     length    (4B) (only for xxxVCHAR)
//     a tag     (1B)
//
//     We need the length and the tag in the end for undo.
//     undo uses the reverse order.
// (2) A committed transaction finishes with NL_COMMIT
//     
class NvmLog {
  public:       
    NvmLog_Log  nl_logbuf_;
    NvmLog_Log::nl_log_pointer_t  nl_log_tx_pos_;

    void **     nl_node_to_del_;
    int         nl_cap_node_to_del_;
    int         nl_num_node_to_del_;

    // Volatile redo records
    typedef struct NlVolatileRedoRec {
      unsigned char      tag_;
      unsigned int       len_;
      void *             addr_;
      unsigned long long value_;
    } NlVolatileRedoRec;

    NlVolatileRedoRec *nl_redo_rec_;
    int                nl_cap_redo_rec_;
    int                nl_num_redo_rec_;

    char              *nl_vchar_buf_;
    int                nl_cap_vchar_buf_;
    int                nl_num_vchar_buf_;

  public:       
    NvmLog() {}

    ~NvmLog() {}

  public:
    // init should be protected by high level logging
    void init()
    {
      nl_logbuf_.initLog();
      nl_logbuf_.prepareLogforWriting();

      nl_cap_node_to_del_= 64;
      nl_node_to_del_= (void **)malloc(sizeof(void*)*nl_cap_node_to_del_);
      if (nl_node_to_del_ == NULL) {perror("malloc"); exit(1);}

      nl_cap_redo_rec_= 64;
      nl_redo_rec_= (NlVolatileRedoRec *)malloc(sizeof(NlVolatileRedoRec)*nl_cap_redo_rec_);
      if (nl_redo_rec_==NULL) {perror("malloc"); exit(1);}

      nl_cap_vchar_buf_=  1024;
      nl_vchar_buf_= (char *)malloc(nl_cap_vchar_buf_);
      if (nl_vchar_buf_==NULL) {perror("malloc"); exit(1);}
    }

    NlVolatileRedoRec *getRedoRec()
    {
      if (nl_num_redo_rec_ == nl_cap_redo_rec_) {
        nl_cap_redo_rec_ *= 2;
        nl_redo_rec_= (NlVolatileRedoRec *)realloc(nl_redo_rec_, 
                                sizeof(NlVolatileRedoRec)*nl_cap_redo_rec_);
        if (nl_redo_rec_==NULL) {perror("realloc"); exit(1);}
      }
      return &(nl_redo_rec_[nl_num_redo_rec_++]);
    }

    char *getVcharBuf(int len)
    {
      while (nl_num_vchar_buf_+len > nl_cap_vchar_buf_) {
        nl_cap_vchar_buf_ *= 2;
        nl_vchar_buf_= (char *)realloc(nl_vchar_buf_, nl_cap_vchar_buf_);
        if (nl_vchar_buf_==NULL) {perror("realloc"); exit(1);}
      }
      char *p= nl_vchar_buf_+nl_num_vchar_buf_;
      nl_num_vchar_buf_ += len;
      return p;
    }

    void startMiniTransaction()
    {
      nl_logbuf_.getLogCurPos(&nl_log_tx_pos_);  // for processing abort
      nl_num_node_to_del_= 0;
      nl_num_redo_rec_= 0;
      nl_num_vchar_buf_= 0;
    }

    void commitMiniTransaction()
    {
      char buffer[1];

      // write redo log records
      for (int ii=0; ii<nl_num_redo_rec_; ii++) {
        char p[64];
        NlVolatileRedoRec *np= (NlVolatileRedoRec *)&nl_redo_rec_[ii];
        switch (np->tag_) {
        case NL_REDO1B: // 1 + (addr)8 + 1 + 1
               p[0]= NL_REDO1B;
               *((void **)(&p[1])) = np->addr_;
               *((unsigned char *)(&p[1+8])) = (unsigned char)(np->value_);
               p[1+8+1]= NL_REDO1B;
               nl_logbuf_.writeLog(p, 1 + 8 + 1 + 1);
               break;
        case NL_REDO2B: // 1 + (addr)8 + 2 + 1
               p[0]= NL_REDO2B;
               *((void **)(&p[1])) = np->addr_;
               *((unsigned short *)(&p[1+8])) = (unsigned short)(np->value_);
               p[1+8+2]= NL_REDO2B;
               nl_logbuf_.writeLog(p, 1 + 8 + 2 + 1);
               break;
        case NL_REDO4B: // 1 + (addr)8 + 4 + 1
               p[0]= NL_REDO4B;
               *((void **)(&p[1])) = np->addr_;
               *((unsigned int *)(&p[1+8])) = (unsigned int)(np->value_);
               p[1+8+4]= NL_REDO4B;
               nl_logbuf_.writeLog(p, 1 + 8 + 4 + 1);
               break;
        case NL_REDO8B: // 1 + (addr)8 + 8 + 1
               p[0]= NL_REDO8B;
               *((void **)(&p[1])) = np->addr_;
               *((unsigned long long *)(&p[1+8])) = (unsigned long long)(np->value_);
               p[1+8+8]= NL_REDO8B;
               nl_logbuf_.writeLog(p, 1 + 8 + 8 + 1);
               break;
        case NL_REDOVCHAR: // 1 + (addr)8 + (len)4 + value[len] + (len)4 + 1
               p[0]= NL_REDOVCHAR;
               *((void **)(&p[1])) = np->addr_;
               *((unsigned int *)(&p[1+8])) = np->len_;
               nl_logbuf_.writeLog(p, 1+8+4);

               nl_logbuf_.writeLog((char *)(np->value_), np->len_);

               *((unsigned int *)(&p[0])) = np->len_;
               p[4]= NL_REDOVCHAR;
               nl_logbuf_.writeLog(p, 4 + 1);
               break;
        }
      } // end of for

      // write del node to log
      if (nl_num_node_to_del_ > 0) {
        char p[8];
        p[0]= NL_DELNODE;   // write command
        *((int *)(&p[1])) = nl_num_node_to_del_; // write num
        nl_logbuf_.writeLog(p, 1+4);
        nl_logbuf_.writeLog((char *)nl_node_to_del_, 
                   nl_num_node_to_del_*sizeof(void*)); // write pointers
        p[1+4]= NL_DELNODE;
        nl_logbuf_.writeLog(p+1, 4+1);  // write num, command
      }

      // write commit
      if (!nl_logbuf_.isSameAsCurPos(&nl_log_tx_pos_)) {
         // write commit only when there are log records
         buffer[0]= NL_COMMIT;
         nl_logbuf_.writeLog(buffer, 1);
         nl_logbuf_.flushLog();
      }

      // actual writes for the redo records
      for (int ii=0; ii<nl_num_redo_rec_; ii++) {
        NlVolatileRedoRec *np= (NlVolatileRedoRec *)&nl_redo_rec_[ii];
        switch (np->tag_) {
        case NL_REDO1B: // 1 + (addr)8 + 1 + 1
               *((unsigned char *)np->addr_)= (unsigned char)(np->value_);
               break;
        case NL_REDO2B: // 1 + (addr)8 + 2 + 1
               *((unsigned short *)np->addr_)= (unsigned short)(np->value_);
               break;
        case NL_REDO4B: // 1 + (addr)8 + 4 + 1
               *((unsigned int *)np->addr_)= (unsigned int)(np->value_);
               break;
        case NL_REDO8B: // 1 + (addr)8 + 8 + 1
               *((unsigned long long *)np->addr_)= (unsigned long long)(np->value_);
               break;
        case NL_REDOVCHAR: // 1 + (addr)8 + (len)4 + value[len] + (len)4 + 1
               memcpy(np->addr_, (void *)(np->value_), np->len_);
               break;
        }
      } // end of for

      // actual deletions
      if (nl_num_node_to_del_ > 0) {
        for (int i=0; i<nl_num_node_to_del_; i++)
           nvmpool_free_node(nl_node_to_del_[i]);
      }
    }

    void abortMiniTransaction()
    {
      NvmLog_Log::nl_log_pointer_t backward;
      nl_logbuf_.getLogCurPos(&backward);
      nl_logbuf_.prepareForReverseRead(&backward);

      char p[128];
      void *addr;
      int   len;

      // undo all the changes in reverse order
      char *p_end= nl_log_tx_pos_.next_ptr_;
      p_end= nl_logbuf_.reverseAdjustPtr(p_end);
      while (backward.next_ptr_ != p_end) {
        switch((unsigned char)(backward.next_ptr_[-1])) {
          case NL_NEW1B:
            nl_logbuf_.readLogReverseSkip(&backward, 1 + 8 + 1 + 1);
            break;
          case NL_NEW2B:
            nl_logbuf_.readLogReverseSkip(&backward, 1 + 8 + 2 + 1);
            break;
          case NL_NEW4B:
            nl_logbuf_.readLogReverseSkip(&backward, 1 + 8 + 4 + 1);
            break;
          case NL_NEW8B:
            nl_logbuf_.readLogReverseSkip(&backward, 1 + 8 + 8 + 1);
            break;
          case NL_NEWVCHAR: // 1 + 8 + 4 + len + 4 + 1
            nl_logbuf_.readLogReverse(&backward, p, 4 + 1);
            len= *((int *)(p));
            nl_logbuf_.readLogReverseSkip(&backward, 1 + 8 + 4 + len);
            break;
          case NL_WRITE1B: // 1 + 8 + 1 + 1 + 1
            nl_logbuf_.readLogReverse(&backward, p, 1 + 8 + 1 + 1 + 1);
            addr= *((void **)(p+1));
            *((unsigned char *)addr)= *((unsigned char *)(p+1+8));
            clwb(addr);
            break;
          case NL_WRITE2B: // 1 + 8 + 2 + 2 + 1
            nl_logbuf_.readLogReverse(&backward, p, 1 + 8 + 2 + 2 + 1);
            addr= *((void **)(p+1));
            *((unsigned short *)addr)= *((unsigned short *)(p+1+8));
            clwb2((char *)addr, (char *)addr + 2 - 1);
            break;
          case NL_WRITE4B: // 1 + 8 + 4 + 4 + 1
            nl_logbuf_.readLogReverse(&backward, p, 1 + 8 + 4 + 4 + 1);
            addr= *((void **)(p+1));
            *((unsigned int *)addr)= *((unsigned int *)(p+1+8));
            clwb2((char *)addr, (char *)addr + 4 - 1);
            break;
          case NL_WRITE8B: // 1 + 8 + 8 + 8 + 1
            nl_logbuf_.readLogReverse(&backward, p, 1 + 8 + 8 + 8 + 1);
            addr= *((void **)(p+1));
            *((unsigned long long *)addr)= *((unsigned long long *)(p+1+8));
            clwb2((char *)addr, (char *)addr + 8 - 1);
            break;
          case NL_WRITEVCHAR: // 1 + 8 + 4 + len + len + 4 + 1
            nl_logbuf_.readLogReverse(&backward, p, 4 + 1);
            len= *((int *)(p));
            {char *tempbuf;
               tempbuf= (char *)malloc(1 + 8 + 4 + len + len);
               if (tempbuf == NULL) {perror("malloc"); exit(1);}                  
               nl_logbuf_.readLogReverse(&backward, tempbuf, 1 + 8 + 4 + len + len);
               addr= *((void **)(tempbuf+1));
               memcpy(addr, tempbuf+1+8+4, len);
               clwbmore((char *)addr, ((char *)addr) + len - 1);
               free(tempbuf);
             }
            break;
          case NL_ALLOCNODE: // 1 + 8 + 1
            nl_logbuf_.readLogReverse(&backward, p, 1 + 8 + 1);
            addr= *((void **)(p+1));
            nvmpool_free_node(addr);
            break;
          case NL_NEXT_CHUNK:
            nl_logbuf_.readLogReverseSkip(&backward, 1);
            break;
          default:
            fprintf(stderr, "NvmLog error: invalid log record: %02x\n",
                   (unsigned char)(backward.next_ptr_[-1]));
            exit(1);
        } // end of switch
      } // end of while

      sfence();

      // write abort record
      p[0]= NL_ABORT;
      nl_logbuf_.writeLog(p, 1);

      nl_logbuf_.flushLog();
    }

    void print(void)
    {
      NvmLog_Log::nl_log_pointer_t pos= nl_log_tx_pos_;
      nl_logbuf_.prepareForRead(&pos);

      char p[128];
      void *addr;
      int   len;

      char *log_write_end= nl_logbuf_.getLogWriteEndPtr();
      while (pos.next_ptr_ != log_write_end) {
        switch((unsigned char)(pos.next_ptr_[0])) {
          case NL_NEW1B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 1 + 1);
            printf("NEW1B addr=%p new_value=%02x\n",
                    *((void **)(p+1)), 
                    *((unsigned char *)(p+1+8)));
            break;
          case NL_NEW2B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 2 + 1);
            printf("NEW2B addr=%p new_value=%04x\n",
                    *((void **)(p+1)), 
                    *((unsigned short *)(p+1+8)));
            break;
          case NL_NEW4B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 4 + 1);
            printf("NEW4B addr=%p new_value=%08x\n",
                    *((void **)(p+1)), 
                    *((unsigned int *)(p+1+8)));
            break;
          case NL_NEW8B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 8 + 1);
            printf("NEW8B addr=%p new_value=%016llx\n",
                    *((void **)(p+1)), 
                    *((unsigned long long *)(p+1+8)));
            break;
          case NL_NEWVCHAR: // 1 + 8 + 4 + len + 4 + 1
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 4);
            len= *((int *)(p+1+8));
            printf("NEWVCHAR addr=%p length=%d\n",
                    *((void **)(p+1)), len);
            nl_logbuf_.readLogSkip(&pos, len + 4 + 1);
            break;
          case NL_WRITE1B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 1 + 1 + 1);
            printf("WRITE1B addr=%p old_value=%02x new_value=%02x\n",
                    *((void **)(p+1)), 
                    *((unsigned char *)(p+1+8)),
                    *((unsigned char *)(p+1+8+1)));
            break;
          case NL_WRITE2B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 2 + 2 + 1);
            printf("WRITE2B addr=%p old_value=%04x new_value=%04x\n",
                    *((void **)(p+1)), 
                    *((unsigned short *)(p+1+8)),
                    *((unsigned short *)(p+1+8+2)));
            break;
          case NL_WRITE4B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 4 + 4 + 1);
            printf("WRITE4B addr=%p old_value=%08x new_value=%08x\n",
                    *((void **)(p+1)), 
                    *((unsigned int *)(p+1+8)),
                    *((unsigned int *)(p+1+8+4)));
            break;
          case NL_WRITE8B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 8 + 8 + 1);
            printf("WRITE8B addr=%p old_value=%016llx new_value=%016llx\n",
                    *((void **)(p+1)), 
                    *((unsigned long long *)(p+1+8)),
                    *((unsigned long long *)(p+1+8+8)));
            break;
          case NL_WRITEVCHAR: // 1 + 8 + 4 + len + len + 4 + 1
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 4);
            len= *((int *)(p+1+8));
            printf("WRITEVCHAR addr=%p length=%d\n",
                    *((void **)(p+1)), len);
            nl_logbuf_.readLogSkip(&pos, len + len + 4 + 1);
            break;
          case NL_REDO1B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 1 + 1);
            printf("REDO1B addr=%p new_value=%02x\n",
                    *((void **)(p+1)),
                    *((unsigned char *)(p+1+8)));
            break;
          case NL_REDO2B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 2 + 1);
            printf("REDO2B addr=%p new_value=%04x\n",
                    *((void **)(p+1)),
                    *((unsigned short *)(p+1+8)));
            break;
          case NL_REDO4B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 4 + 1);
            printf("REDO4B addr=%p new_value=%08x\n",
                    *((void **)(p+1)),
                    *((unsigned int *)(p+1+8)));
            break;
          case NL_REDO8B:
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 8 + 1);
            printf("REDO8B addr=%p new_value=%016llx\n",
                    *((void **)(p+1)),
                    *((unsigned long long *)(p+1+8)));
            break;
          case NL_REDOVCHAR: // 1 + 8 + 4 + len + 4 + 1
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 4);
            len= *((int *)(p+1+8));
            printf("REDOVCHAR addr=%p length=%d\n",
                    *((void **)(p+1)), len);
            nl_logbuf_.readLogSkip(&pos, len + 4 + 1);
            break;
          case NL_ALLOCNODE: // 1 + 8 + 1
            nl_logbuf_.readLog(&pos, p, 1 + 8 + 1);
            printf("ALLOCNODE addr=%p\n", *((void **)(p+1)));
            break;
          case NL_DELNODE: // 1 + num + ptr[] + num + 1
            {int num;
             nl_logbuf_.readLog(&pos, p, 1 + 4);
             num= *((int *)(p+1));
             printf("DELNODE num=%d:", num);
             for (int i=0; i<num; i++) {
                nl_logbuf_.readLog(&pos, p, sizeof(void*));
                printf(" %p", *((void **)p));
             }
             printf("\n");
             nl_logbuf_.readLogSkip(&pos, 4 + 1);
            }
            break;
          case NL_NEXT_CHUNK:
            printf("NEXT_CHUNK\n"); nl_logbuf_.readLogSkip(&pos, pos.nextline_ptr_ - pos.next_ptr_);
            break;
          case NL_COMMIT:
            printf("COMMIT\n"); nl_logbuf_.readLogSkip(&pos, 1);
            break;
          case NL_ABORT:
            printf("ABORT\n"); nl_logbuf_.readLogSkip(&pos, 1);
            break;
          case NL_ONGOING:
            printf("ONGOING\n"); nl_logbuf_.readLogSkip(&pos, 1);
            break;
          default:
            fprintf(stderr, "NvmLog error: invalid log record!\n");
            exit(1);
        } // end of switch
      } // end of while
    }
        
  public:
    // log enhanced writes
    // 1 + (addr)8 + 8 + 8 + 1
    void write8B(void * addr, unsigned long long value)
    {
      // write log record
      char p[64];
      p[0]= NL_WRITE8B;
      *((void **)(&p[1])) = addr;
      *((unsigned long long *)(&p[1+8])) = *((unsigned long long *)addr);
      *((unsigned long long *)(&p[1+8+8])) = value;
      p[1+8+8+8]= NL_WRITE8B;
      char *p_next= p + 1 + 8 + 8 + 8 + 1;

      // write and flush log record
      nl_logbuf_.writeLog(p, p_next - p);
      nl_logbuf_.flushLog();

      // perform data write operation
      *((unsigned long long *)addr)= value;
    }

    // 1 + (addr)8 + 4 + 4 + 1
    void write4B(void * addr, unsigned int value)
    {
      // write log record
      char p[64];
      p[0]= NL_WRITE4B;
      *((void **)(&p[1])) = addr;
      *((unsigned int *)(&p[1+8])) = *((unsigned int *)addr);
      *((unsigned int *)(&p[1+8+4])) = value;
      p[1+8+4+4]= NL_WRITE4B;
      char *p_next= p + 1 + 8 + 4 + 4 + 1;

      // write and flush log record
      nl_logbuf_.writeLog(p, p_next - p);
      nl_logbuf_.flushLog();

      // perform data write operation
      *((unsigned int *)addr)= value;
    }

    // 1 + (addr)8 + 2 + 2 + 1
    void write2B(void * addr, unsigned short value)
    {
      // write log record
      char p[64];
      p[0]= NL_WRITE2B;
      *((void **)(&p[1])) = addr;
      *((unsigned short *)(&p[1+8])) = *((unsigned short *)addr);
      *((unsigned short *)(&p[1+8+2])) = value;
      p[1+8+2+2]= NL_WRITE2B;
      char *p_next= p + 1 + 8 + 2 + 2 + 1;

      // write and flush log record
      nl_logbuf_.writeLog(p, p_next - p);
      nl_logbuf_.flushLog();

      // perform data write operation
      *((unsigned short *)addr)= value;
    }

    // 1 + (addr)8 + 1 + 1 + 1
    void write1B(void * addr, unsigned char value)
    {
      // write log record
      char p[64];
      p[0]= NL_WRITE1B;
      *((void **)(&p[1])) = addr;
      *((unsigned char *)(&p[1+8])) = *((unsigned char *)addr);
      *((unsigned char *)(&p[1+8+1])) = value;
      p[1+8+1+1]= NL_WRITE1B;
      char *p_next= p + 1 + 8 + 1 + 1 + 1;

      // write and flush log record
      nl_logbuf_.writeLog(p, p_next - p);
      nl_logbuf_.flushLog();

      // perform data write operation
      *((unsigned char *)addr)= value;
    }

    // 1 + (addr)8 + (len)4 + old value[len] + new value[len] + (len)4 + 1
    void writeVchar(void * addr, int len, const char *value)
    {
      // write log record
      char p[64];
      p[0]= NL_WRITEVCHAR;
      *((void **)(&p[1])) = addr;
      *((unsigned int *)(&p[1+8])) = (unsigned int)len;
      nl_logbuf_.writeLog(p, 1+8+4);

      nl_logbuf_.writeLog((char *)addr, len);
      nl_logbuf_.writeLog(value, len);

      *((unsigned int *)(&p[0])) = (unsigned int)len;
      p[4]= NL_WRITEVCHAR;
      char *p_next= p + 4 + 1;

      // write and flush log record
      nl_logbuf_.writeLog(p, p_next - p);
      nl_logbuf_.flushLog();

      // perform data write operation
      memcpy(addr, value, len);
    }
        
    // 1 + (addr)8 + 8 + 1
    void new8B(void * addr, unsigned long long value)
    {
      // write log record
      char p[64];
      p[0]= NL_NEW8B;
      *((void **)(&p[1])) = addr;
      *((unsigned long long *)(&p[1+8])) = value;
      p[1+8+8]= NL_NEW8B;
      char *p_next= p + 1 + 8 + 8 + 1;
    
      // write log record, no need to flush
      nl_logbuf_.writeLog(p, p_next - p);

      // perform data write operation
      *((unsigned long long *)addr)= value;
    }

    // 1 + (addr)8 + 4 + 1
    void new4B(void * addr, unsigned int value)
    {
      // write log record
      char p[64];
      p[0]= NL_NEW4B;
      *((void **)(&p[1])) = addr;
      *((unsigned int *)(&p[1+8])) = value;
      p[1+8+4]= NL_NEW4B;
      char *p_next= p + 1 + 8 + 4 + 1;

      // write log record, no need to flush
      nl_logbuf_.writeLog(p, p_next - p);

      // perform data write operation
      *((unsigned int *)addr)= value;
    }

    // 1 + (addr)8 + 2 + 1
    void new2B(void * addr, unsigned short value)
    {
      // write log record
      char p[64];
      p[0]= NL_NEW2B;
      *((void **)(&p[1])) = addr;
      *((unsigned short *)(&p[1+8])) = value;
      p[1+8+2]= NL_NEW2B;
      char *p_next= p + 1 + 8 + 2 + 1;

      // write log record, no need to flush
      nl_logbuf_.writeLog(p, p_next - p);

      // perform data write operation
      *((unsigned short *)addr)= value;
    }

    // 1 + (addr)8 + 1 + 1
    void new1B(void * addr, unsigned char value)
    {
      // write log record
      char p[64];
      p[0]= NL_NEW1B;
      *((void **)(&p[1])) = addr;
      *((unsigned char *)(&p[1+8])) = value;
      p[1+8+1]= NL_NEW1B;
      char *p_next= p + 1 + 8 + 1 + 1;

      // write log record, no need to flush
      nl_logbuf_.writeLog(p, p_next - p);

      // perform data write operation
      *((unsigned char *)addr)= value;
    }

    // 1 + (addr)8 + (len)4 + value[len] + (len)4 + 1
    void newVchar(void * addr, int len, const char *value)
    {
      // write log record
      char p[64];
      p[0]= NL_NEWVCHAR;
      *((void **)(&p[1])) = addr;
      *((unsigned int *)(&p[1+8])) = (unsigned int)len;
      nl_logbuf_.writeLog(p, 1+8+4);

      nl_logbuf_.writeLog(value, len);

      *((unsigned int *)(&p[0])) = (unsigned int)len;
      p[4]= NL_NEWVCHAR;
      char *p_next= p + 4 + 1;

      // write log record, no need to flush
      nl_logbuf_.writeLog(p, p_next - p);

      // perform data write operation
      memcpy(addr, value, len);
    }

    void redoWrite8B(void * addr, unsigned long long value)
    { 
      NlVolatileRedoRec *p= getRedoRec();
      p->tag_= NL_REDO8B;
      p->addr_= addr;
      p->value_= value;
    }
    
    void redoWrite4B(void * addr, unsigned int value)
    { 
      NlVolatileRedoRec *p= getRedoRec();
      p->tag_= NL_REDO4B;
      p->addr_= addr;
      p->value_= value;
    }
    
    void redoWrite2B(void * addr, unsigned short value)
    { 
      NlVolatileRedoRec *p= getRedoRec();
      p->tag_= NL_REDO2B;
      p->addr_= addr;
      p->value_= value;
    }
      
    void redoWrite1B(void * addr, unsigned char value)
    { 
      NlVolatileRedoRec *p= getRedoRec();
      p->tag_= NL_REDO1B;
      p->addr_= addr;
      p->value_= value;
    }

    void redoWriteVchar(void * addr, int len, const char *value)
    { 
      if (len > 0) {
        NlVolatileRedoRec *p= getRedoRec();
        p->tag_= NL_REDOVCHAR;
        p->len_= len;
        p->addr_= addr;

        char *chp= getVcharBuf(len);
        memcpy(chp, value, len);
        p->value_= (unsigned long long)chp;
      }
    }
    
    // record node allocation request
    void* allocNode(int size)
    { 
      void *ptr = nvmpool_alloc_node(size);

      // write log record
      char p[64];
      p[0]= NL_ALLOCNODE;   // 1B command
      *((void **)(&p[1])) = ptr; // 8B address
      p[1+8]= NL_ALLOCNODE; // 1B command

      // write log record, flush
      nl_logbuf_.writeLog(p, 1+8+1);
      nl_logbuf_.flushLog();

      return ptr;
    }

    // record node deletion request
    void delNode(void *p)
    {
      if (nl_num_node_to_del_==nl_cap_node_to_del_) {
         nl_cap_node_to_del_ *= 2;
         nl_node_to_del_= (void **)realloc(nl_node_to_del_, 
                        sizeof(void*)*nl_cap_node_to_del_);
         if (nl_node_to_del_ == NULL) {perror("realloc"); exit(1);}
      }
      nl_node_to_del_[nl_num_node_to_del_++]= p;
    }
        
}; // NvmLog

extern void nvmLogInit(int num_workers);

extern NvmLog *the_nvm_logs;
extern thread_local int worker_id;  /* in Thread Local Storage */

#define mylog  (the_nvm_logs[worker_id])

#endif /* _NVM_COMMON_H */
