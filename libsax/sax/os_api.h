#ifndef __OS_API_H_2009_06__
#define __OS_API_H_2009_06__

/// @defgroup OS_API dozens of API to make code OS-independent.
/// @author  Qingshan Luo (qluo.cn@gmail.com)
/// @version  0.36
/// @date    June 2009, APR. 2010, Aug. 2010, Dec. 2010
/// @{
/// Four kinds of objects are implemented:
///  -# g_mutex_t: mutex object for thread
///  -# g_thread_t: lightweighed thread object
///  -# g_share_t: R/W locker for thread
///  -# g_spin_t: R/W locker based on atomic operators
///  -# g_cond_t: condition object for thread
/// 
/// Three types of IO interfaces are designed for different tasks:
///  -# f64_t: Random Access on large file (>=4G), which provides 
/// some usual operations for Random Accessing on large files. 
/// Specially, f64lock is also implemented for process-safe sharing.
///  -# shm_t: Map addresses for a file into memory, which provides
/// the method for data sharing among processes.
///  -# mix_t: A mixed application for F64 and SHM, it binds mmap() 
/// and general file IO together, and make it possible to use the 
/// memory first, then file. It is useful for those cases wishing 
/// to adopt mmap(), but can't estimate the memory needed. This toy 
/// handles the overflowed part with the general file IO.
/// @}

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

//-------------------------------------------------------------------------
//------------------ (a) common part for string routines ------------------
//-------------------------------------------------------------------------
/// @brief Copy src to string dst of size siz (strlcpy).
/// @param dst the destinating string.
/// @param src the source string.
/// @param siz size (capacity) of dst.
int g_strlcpy(char *dst, const char *src, int siz);

/// @brief Appends src to string dst of size siz (strlcat).
/// @param dst the destinating string.
/// @param src the source string.
/// @param siz size (capacity) of dst.
int g_strlcat(char *dst, const char *src, int siz);

/// @brief Compares the first @var{n} bytes of two strings, 
/// returning a value as @code{strcmp}.
int g_strncmp(const char *s1, const char *s2, int n);

/// @brief A case-insensitive @code{strcmp}.
int g_stricmp(const char *s1, const char *s2);

/// @brief A case-insensitive @code{strncmp}.
int g_strnicmp(const char *s1, const char *s2, int n);

/// @brief duplicate a string by malloc
char *g_strdup(const char *s);

/// @brief an alias of free, keep malloc/free the same version
/// which is benifitial when handling .DLL, .so etc.
void g_release(void *p);

/// @brief strchr from the right
char *g_strrchr(const char *s, char c);

/// @brief trim the right space characters
void g_strrtrim(char *s);

/// @brief trim the space characters on both sides
void g_strtrim(char *s);

/// @brief string Wildcard Matching (* and ?)
int g_strmatch(const char *str, const char *pattern, 
	int is_case_sensitive, char terminator); // 1, '\0'

/// @brief an alias for snprintf().
int g_snprintf(char *buf, int sz, const char *fmt, ...);


/// @brief URL-decode/encode, see RFC 1866 section 8.2.1
/// http://ftp.ics.uci.edu/pub/ietf/html/rfc1866.txt
int g_url_decode(const char *src, int src_len, char *dst,
	int dst_len, int is_form_url_encoded);
int g_url_encode(const char *src, char *dst, int dst_len);


/// @brief byte-decode/encode to 64 bits string (not base64)
int g_b64_decode(const char *src, int src_len, 
	unsigned char *dst, int dst_len);
int g_b64_encode(const unsigned char *src, 
	int src_len, char *dst, int dst_len);


/// @brief Convert a local path like ./foo into an Internet
///        URL like file:///path/to/foo
int g_path_to_url(const char *path, char url[], int size);

/// @brief Convert a url like file:///path/to/foo into 
///        a local path like /path/to/foo
int g_url_to_path(const char *url, char path[], int size);

/// @brief Win32 API has built-in function for command line
/// parsing. The function is named CommandLineToArgvW(). 
///
/// This function simulates the CommandLineToArgvW() but with
/// ANSI version. It builds standard C-style argc/argv pair
/// from command line string.
///
/// @note Remember to call free(argv) to release memory later.
char **g_cmd_to_argv(const char *cmd, int* argc);

//-------------------------------------------------------------------------
//------------------- (b) API for file/directory access -------------------
//-------------------------------------------------------------------------
/// @brief create a directory
/// @param dir the specified directory path
/// @return flag for failed or successful
int g_create_dir(const char *dir);

/// @brief delete a directory
/// @param dir the specified directory path
/// @return flag for failed or successful
int g_delete_dir(const char *dir);

/// @brief check the existence of a directory
/// @param dir the specified directory path
/// @retval 0 a flag for lost
/// @retval 1 a flag for existing
int g_exists_dir(const char *dir);

/// @brief create directory recursively
int g_mkdir_recur(const char *dir);

/// @brief check the existence of a file
/// @param fn the handling file name
/// @retval 0 a flag for lost
/// @retval 1 a flag for existing
int g_exists_file(const char *fn);

/// @brief delete a file from disk
/// @param fn the specified path
/// @return flag for failed or successful
int g_delete_file(const char *fn);

/// @brief copy a src file to dst file
/// @param src the path of source file
/// @param dst the path of destination
/// @return flag for failed or successful
int g_copy_file(const char *src, const char *dst);

/// @brief move a src file to dst file
/// @param src the path of source file
/// @param dst the path of destination
/// @return flag for failed or successful
int g_move_file(const char *src, const char *dst);

/// @brief Turn a relative path name into a full path name.
/// @param rel a relative path name
/// @param full a full path name
/// @param size size (capacity) of full[].
/// @retval 0 a flag for failed
/// @retval 1 a flag for successful
int g_full_path(const char *rel, char full[], int size);

/// POSIX dirent interface
struct g_dirent_t {
	int64_t fsize;
	int64_t mtime;
	int  is_dir;
	char dname[260];
};
typedef struct DIR_HANDLE DIR_HANDLE;

/// Implementation of POSIX opendir/closedir/readdir
DIR_HANDLE *g_opendir(const char *name);
int g_closedir(DIR_HANDLE *ctx);
struct g_dirent_t *g_readdir(DIR_HANDLE *ctx);



//-------------------------------------------------------------------------
//------------------- (x) Hardware Information ----------------------------
//-------------------------------------------------------------------------

/// @brief retrieve the disk size information
int g_disk_info(const char *dir, uint64_t *_free, uint64_t *_total);

/// @brief retrieve the memory information
int g_mem_phys(uint64_t *_free, uint64_t *_total);
int g_mem_virt(uint64_t *_free, uint64_t *_total);

/// @brief retrieve the system load average (unit: 1/10000) 
int g_load_avg(void);

/// @brief retrieve the CPU usage (unit: 1/10000)
/// @note it should be called at least twice:
/// <pre>
/// char tick[16]; // magic buffer
/// g_cpu_usage(tick, NULL);   // init
/// ... ... // run biz or just sleep()
/// g_cpu_usage(tick, &usage); // calc
/// ... ... // run biz or just sleep()
/// g_cpu_usage(tick, &usage); // calc again
/// </pre>
int g_cpu_usage(char tick[16], int *out);

//-------------------------------------------------------------------------
//--------------------- (c) API for file lock access ----------------------
//-------------------------------------------------------------------------
/// defines some operations for f64lock(), g_flock() etc:
/// - flock and fcntl aren't part of Standard C++ per ISO/IEC 14882:2003.
/// - Note that flock doesn't support NFS, in contrast, fcntl function 
/// satisfy POSIX Standard, which creates a daemon to support NFS too.
/// We define a new flock (based on fcntl) here:
#ifndef LOCK_SH
#	define LOCK_SH 1 ///< sharing mode
#	define LOCK_EX 2 ///< exclusive mode
#	define LOCK_NB 4 ///< do not wait, immediately
#	define LOCK_UN 8 ///< unlock
#endif//LOCK_SH

/// @brief locks the specified file for exclusive/share access.
/// @param fd the file no.
/// @param op one of (LOCK_SH, LOCK_EX, LOCK_NB, LOCK_UN)
/// @retval 0 a flag for failed
/// @retval 1 a flag for successful
int g_flock(int fd, int op);

/// @brief locks the specified file with the pointer "FILE *".
/// @param fp the file handle.
/// @param op one of (LOCK_SH, LOCK_EX, LOCK_NB, LOCK_UN)
/// @retval 0 a flag for failed
/// @retval 1 a flag for successful
int g_flock2(void* fp, int op);


//-------------------------------------------------------------------------
//-------- (d) API for thread/mutex/spin/semp/condition objects -----------
//-------------------------------------------------------------------------
typedef struct g_mutex_t g_mutex_t;

/// @brief create an instantce of mutex (recursive!):
/// @return a handle of struct g_mutex_t
g_mutex_t *g_mutex_init(void);

/// @brief free an instance of mutex
/// @param p a handle of struct g_mutex_t
void g_mutex_free(g_mutex_t *p);

/// @brief intended for use only inside assert() statements
/// @param p a handle of struct g_mutex_t
/// @return 1 for holding, 0 for not held.
int  g_mutex_held(g_mutex_t *p);

/// @brief begin to enter a mutex
/// @param p a handle of struct g_mutex_t
void g_mutex_enter(g_mutex_t *p);

/// @brief have a try to enter a mutex
int  g_mutex_try_enter(g_mutex_t *p, double sec);

/// @brief exits a mutex that was previously entered by the same thread.
/// @param p a handle of struct g_mutex_t
void g_mutex_leave(g_mutex_t *p);

//-------------------------------------------------------------------------

typedef void* g_thread_t;

/// @brief (outer) create and start a thread.
g_thread_t g_thread_start(void* (*func)(void *), void *user);

/// @brief (outer) wait for the thread to finish.
int g_thread_join(g_thread_t th, long *val);

/// @brief (outer) create and start a detached thread.
int g_thread_start_detached(void* (*func)(void *), void *user);

/// @brief (inner) suspend execution of the current thread.
int g_thread_sleep(double sec);

/// @brief (inner) yield the processor from the running thread.
void g_thread_yield();

/// @brief (inner) put a pause instruction, hinting the processor that "this is a spin loop."
void g_thread_pause();

/// @brief (inner) terminate the running thread.
void g_thread_exit(long val);

/// @brief (inner) retrieve the thread ID.
long g_thread_id();

/// @brief (inner) bind a cpu or a name.
int g_thread_bind(int cpu, const char *name);

/// @brief (inner) retrieve the process ID.
long g_process_id();

/// @brief three atom-operations:
long g_lock_set(long *ptr, long new_val);
long g_lock_add(long *ptr, long inc_val);
long g_ifeq_set(long *ptr, long cmp, long new_val);


/// @brief some API for current tick:
int64_t g_now_hr(); // high resolution, cpu tick for relative usec
int64_t g_now_us(); // GetSystemTimeAsFileTime()/gettimeofday(): usec
int64_t g_now_ms(); // an alias of: g_now_us()/1000

int g_timezone();
uint64_t g_now_uid(); // unique tick as ID

/// @brief convert time_t to struct tm
struct tm* g_localtime(time_t _sec, struct tm *_tp, long offset);

//-------------------------------------------------------------------------
typedef struct g_share_t g_share_t;

/// @brief initialize
g_share_t *g_share_init(void);

/// @brief deconstruct
void g_share_free(g_share_t *s);


/// @brief try to get a writer lock.
void g_share_lockw(g_share_t *s);
int  g_share_try_lockw(g_share_t *s, double sec);

/// @brief try to get a reader lock.
void g_share_lockr(g_share_t *s);
int  g_share_try_lockr(g_share_t *s, double sec);

/// @brief release the lock.
void g_share_unlock(g_share_t *s);

//-------------------------------------------------------------------------
typedef struct g_spin_t g_spin_t;

/// @brief initialize
g_spin_t* g_spin_init();

/// @brief deconstruct
void g_spin_free(g_spin_t* s);

/// @brief lock a spin lock
void g_spin_enter(g_spin_t* s, int spin);

/// @brief release a spin lock
void g_spin_leave(g_spin_t* s);

//-------------------------------------------------------------------------
typedef struct g_spin_rw_t g_spin_rw_t;

/// @brief initialize
g_spin_rw_t* g_spin_rw_init(int prefer_writer);

/// @brief deconstruct
void g_spin_rw_free(g_spin_rw_t* s);

/// @brief try to get a writer lock.
void g_spin_lockw(g_spin_rw_t* s, int spin);
int  g_spin_try_lockw(g_spin_rw_t* s, double sec, int spin);

/// @brief try to get a reader lock.
void g_spin_lockr(g_spin_rw_t* s, int spin);
int  g_spin_try_lockr(g_spin_rw_t* s, double sec, int spin);

/// @brief release the lock.
void g_spin_unlockw(g_spin_rw_t* s);
void g_spin_unlockr(g_spin_rw_t* s);

//-------------------------------------------------------------------------

typedef struct g_sema_t g_sema_t;

/// @brief create a semaphore object
g_sema_t *g_sema_init(uint32_t cap, uint32_t cnt);

/// @brief destroy a semaphore object
void g_sema_free(g_sema_t *s);

/// @brief send a signal
int	g_sema_post(g_sema_t *s);

/// @brief wait a signal
int	g_sema_wait(g_sema_t *s, double sec); // sec<=0 for INFINITE

//-------------------------------------------------------------------------

typedef struct g_cond_t g_cond_t;

/// @brief initialize
g_cond_t *g_cond_init(void);

/// @brief deconstruct
void g_cond_free(g_cond_t *gc);

/// @brief Wait for the signal.
int g_cond_wait(g_cond_t *gc, g_mutex_t *mtx, double sec);
  
/// @brief Send the wake-up signal to another waiting thread.
void g_cond_signal(g_cond_t *gc);

/// @brief Send the wake-up signal to all waiting threads.
void g_cond_broadcast(g_cond_t *gc);

//-------------------------------------------------------------------------
//------ (e) API for Random Access on large file (>=4G, 32bit OS) ---------
//-------------------------------------------------------------------------
typedef struct f64_t f64_t;

f64_t *f64open(const char *fn, int create);
int f64close(f64_t *f);

int f64read(f64_t *f, void *ptr, int amt, int64_t offset);
int f64write(f64_t *f, const void *ptr, int amt, int64_t offset);

int f64tell(f64_t *f, int64_t *offset);
int f64sync(f64_t *f, int flag);

int f64trunc(f64_t *f, int64_t size);
int64_t f64size(f64_t *f);

int f64lock(f64_t *f, int op);


//-------------------------------------------------------------------------
//------------- (f) API for memory mapping among process ------------------
//-------------------------------------------------------------------------

/// a structure for sharing memory mapping
typedef struct shm_t {
	uint8_t *ptr; ///< mapping address for shm_t
	uint32_t len; ///< size of the memory space
	void  *owner; ///< HANDLE of hFileMapping (for Win32)
} shm_t;

// The meaningful base address to sharing pointer among processes.
#define SHM_NULL ((void *)   0) ///< determined by OS, alias for NULL
#define SHM_LOAD ((void *)-1ul) ///< load the old address if possible

/// @brief Map addresses for a file from 'start' with a given length.
/// @param fn the file name, can be NULL for a ananimous object.
/// @param start the base (starting) address to be mapping. 
///   There are two special values for application:@n
///    -# start=SHM_NULL, automatical address dy OS. @n
///    -# start=SHM_LOAD, try old address, if failed and not forced, 
///        then try SHM_NULL. struct mb_man_t will be happy for it!@n
/// @param len the desired length of mapping space.
/// @param forced a flag determining whether return address as expected, 
///        it is no use for SHM_NULL.
/// @return pointer of the destination mapping object.
/// @note we can specify a given address for "start", repeat testing
/// with increasing by Allocation Granularity until it is OKay.
/// - Linux's mmap require: n*0x1000
/// - Win32's MapViewOfFileEx: n*0x10000
/// @remark an unpredictable address will be returned except [forced].
shm_t *g_shm_open(
	const char *fn, void *start, uint32_t len, int forced);

/// @brief Deallocate any mapping for the region.
/// @param map a handle of struct shm_t
/// @return 1 for close okay, 0 for failed.
int g_shm_close(shm_t *map);

/// @brief Synchronize a region to the file it is mapping.
/// @param map a handle of struct shm_t
/// @param s starting offset
/// @param c number of bytes
/// @return 1 for sync okay, 0 for failed.
int g_shm_sync(const shm_t *map, uint32_t s, uint32_t c);

/// @brief get the system's memory allocation granularity.
/// @return memory allocation granularity
int g_shm_unit(void);

/// @brief Allocate numbers of entire page(g_shm_unit()) memory.
/// @param pages number of page would like to allocate.
/// @return NULL for failed, otherwise the returning address is align
///         with system page boundary.
void* g_shm_alloc_pages(uint32_t pages);

/// @brief Deallocate memory that allocated from g_shm_alloc_pages().
/// @param ptr memory address to free.
void g_shm_free_pages(void* ptr);

//-------------------------------------------------------------------------
//------------- (g) API for mixed file (shm_t + f64_t) --------------------
//-------------------------------------------------------------------------
typedef struct m64_t m64_t;

m64_t *m64open(const char *fn, uint32_t xmap);
int m64close(m64_t *m);

int m64read(m64_t *m, void *ptr, int amt, int64_t offset);
int m64write(m64_t *m, const void *ptr, int amt, int64_t offset);

int m64tell(m64_t *m, int64_t *offset);
int m64sync(m64_t *m, int flag);

int m64trunc(m64_t *m, int64_t size);
int64_t m64size(m64_t *m);

int m64lock(m64_t *m, int op);

//-------------------------------------------------------------------------
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //__OS_API_H_2009_06__

