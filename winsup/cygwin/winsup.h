/* winsup.h: main Cygwin header file.

   Copyright 1996, 1997, 1998, 1999, 2000 Cygnus Solutions.

This file is part of Cygwin.

This software is a copyrighted work licensed under the terms of the
Cygwin license.  Please consult the file "CYGWIN_LICENSE" for
details. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define __INSIDE_CYGWIN__

#ifdef __cplusplus

#define alloca(x) __builtin_alloca (x)
#define strlen __builtin_strlen
#define strcpy __builtin_strcpy
#define memcpy __builtin_memcpy
#define memcmp __builtin_memcmp
#ifdef HAVE_BUILTIN_MEMSET
# define memset __builtin_memset
#endif

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ >= 199900L
#define NEW_MACRO_VARARGS
#endif

#include <sys/types.h>
#include <sys/strace.h>
#include <sys/resource.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>

#undef strchr
#define strchr cygwin_strchr
extern "C" inline __stdcall char * strchr(const char * s, int c)
{
register char * __res;
__asm__ __volatile__(
	"movb %%al,%%ah\n"
	"1:\tmovb (%1),%%al\n\t"
	"cmpb %%ah,%%al\n\t"
	"je 2f\n\t"
	"incl %1\n\t"
	"testb %%al,%%al\n\t"
	"jne 1b\n\t"
	"xorl %1,%1\n"
	"2:\tmovl %1,%0\n\t"
	:"=a" (__res), "=r" (s)
	:"0" (c),      "1"  (s));
return __res;
}

#define WIN32_LEAN_AND_MEAN 1
#define _WINGDI_H
#define _WINUSER_H
#define _WINNLS_H
#define _WINVER_H
#define _WINNETWK_H
#define _WINSVC_H
#include <windows.h>
#include <wincrypt.h>
#undef _WINGDI_H
#undef _WINUSER_H
#undef _WINNLS_H
#undef _WINVER_H
#undef _WINNETWK_H
#undef _WINSVC_H

/* The one function we use from winuser.h most of the time */
extern "C" DWORD WINAPI GetLastError (void);

/* Used for runtime OS check/decisions. */
enum os_type {winNT = 1, win95, win98, win32s, unknown};
extern os_type os_being_run;

/* Used to check if Cygwin DLL is dynamically loaded. */
extern int dynamically_loaded;

#define sys_wcstombs(tgt,src,len) \
                    WideCharToMultiByte(CP_ACP,0,(src),-1,(tgt),(len),NULL,NULL)
#define sys_mbstowcs(tgt,src,len) \
                    MultiByteToWideChar(CP_ACP,0,(src),-1,(tgt),(len))

#include <cygwin/version.h>

#define TITLESIZE 1024
#define MAX_USER_NAME 20
#define DEFAULT_UID 500
#define DEFAULT_GID 544

#define MAX_SID_LEN 40
#define MAX_HOST_NAME 256

/* status bit manipulation */
#define __ISSETF(what, x, prefix) \
  ((what)->status & prefix##_##x)
#define __SETF(what, x, prefix) \
  ((what)->status |= prefix##_##x)
#define __CLEARF(what, x, prefix) \
  ((what)->status &= ~prefix##_##x)
#define __CONDSETF(n, what, x, prefix) \
  ((n) ? __SETF (what, x, prefix) : __CLEARF (what, x, prefix))

#include "shared.h"

extern HANDLE hMainThread;
extern HANDLE hMainProc;

#include "debug.h"
#include <sys/cygwin.h>

/********************** Application Interface **************************/

extern "C" per_process __cygwin_user_data; /* Pointer into application's static data */
#define user_data (&__cygwin_user_data)

/* We use the following to test that sizeof hasn't changed.  When adding
   or deleting members, insert fillers or use the reserved entries.
   Do not change this value. */
#define SIZEOF_PER_PROCESS (42 * 4)

/******************* Host-dependent constants **********************/
/* Portions of the cygwin DLL require special constants whose values
   are dependent on the host system.  Rather than dynamically
   determine those values whenever they are required, initialize these
   values once at process start-up. */

class host_dependent_constants
{
 public:
  void init (void);

  /* Used by fhandler_disk_file::lock which needs a platform-specific
     upper word value for locking entire files. */
  DWORD win32_upper;

  /* fhandler_base::open requires host dependent file sharing
     attributes. */
  int shared;
};

extern host_dependent_constants host_dependent;

/* Events/mutexes */
extern HANDLE title_mutex;

/**************************** Convenience ******************************/

#define NO_COPY __attribute__((section(".data_cygwin_nocopy")))

/* Used when treating / and \ as equivalent. */
#define SLASH_P(ch) \
    ({ \
	char __c = (ch); \
	((__c) == '/' || (__c) == '\\'); \
    })

/* Convert a signal to a signal mask */
#define SIGTOMASK(sig)	(1<<((sig) - signal_shift_subtract))
extern unsigned int signal_shift_subtract;

#ifdef NOSTRACE
#define MARK() 0
#else
#define MARK() mark (__FILE__,__LINE__)
#endif

#ifdef NEW_MACRO_VARARGS
# define api_fatal(...) __api_fatal ("%P: *** " __VA_ARGS__)
#else
# define api_fatal(fmt, args...) __api_fatal ("%P: *** " fmt,## args)
#endif

#undef issep
#define issep(ch) (strchr (" \t\n\r", (ch)) != NULL)

#define isdirsep SLASH_P
#define isabspath(p) \
  (isdirsep (*(p)) || (isalpha (*(p)) && (p)[1] == ':' && (!(p)[2] || isdirsep ((p)[2]))))

/******************** Initialization/Termination **********************/

/* cygwin .dll initialization */
void dll_crt0 (per_process *);
extern "C" void __stdcall _dll_crt0 ();

/* dynamically loaded dll initialization */
extern "C" int dll_dllcrt0 (HMODULE, per_process*);

/* dynamically loaded dll initialization for non-cygwin apps */
extern "C" int dll_noncygwin_dllcrt0 (HMODULE, per_process *);

/* exit the program */
extern "C" void __stdcall do_exit (int) __attribute__ ((noreturn));

/* Initialize the environment */
void environ_init (int);

/* Heap management. */
void heap_init (void);
void malloc_init (void);

/* UID/GID */
void uinfo_init (void);

/* various events */
void events_init (void);
void events_terminate (void);

void __stdcall close_all_files (void);

/* Invisible window initialization/termination. */
HWND __stdcall gethwnd (void);
void __stdcall window_terminate (void);

/* Globals that handle initialization of winsock in a child process. */
extern HANDLE wsock32_handle;

/* Globals that handle initialization of netapi in a child process. */
extern HANDLE netapi32_handle;

/* debug_on_trap support. see exceptions.cc:try_to_debug() */
extern "C" void error_start_init (const char*);
extern "C" int try_to_debug ();

extern int cygwin_finished_initializing;

/**************************** Miscellaneous ******************************/

/* File manipulation */
int __stdcall set_process_privileges ();
int __stdcall get_file_attribute (int, const char *, int *,
                                  uid_t * = NULL, gid_t * = NULL);
int __stdcall set_file_attribute (int, const char *, int);
int __stdcall set_file_attribute (int, const char *, uid_t, gid_t, int, const char *);
void __stdcall set_std_handle (int);
int __stdcall writable_directory (const char *file);
int __stdcall stat_dev (DWORD, int, unsigned long, struct stat *);
extern BOOL allow_ntsec;

/* `lookup_name' should be called instead of LookupAccountName.
 * logsrv may be NULL, in this case only the local system is used for lookup.
 * The buffer for ret_sid (40 Bytes) has to be allocated by the caller! */
BOOL __stdcall lookup_name (const char *, const char *, PSID);
char *__stdcall convert_sid_to_string_sid (PSID, char *);
PSID __stdcall convert_string_sid_to_sid (PSID, const char *);
BOOL __stdcall get_pw_sid (PSID, struct passwd *);

unsigned long __stdcall hash_path_name (unsigned long hash, const char *name);
void __stdcall nofinalslash (const char *src, char *dst);
extern "C" char *__stdcall rootdir (char *full_path);

void __stdcall mark (const char *, int);

extern "C" int _spawnve (HANDLE hToken, int mode, const char *path,
			 const char *const *argv, const char *const *envp);

extern void __stdcall exec_fixup_after_fork ();

class _pinfo;
/* For mmaps across fork(). */
int __stdcall recreate_mmaps_after_fork (void *);
void __stdcall set_child_mmap_ptr (_pinfo *);

/* String manipulation */
char *__stdcall strccpy (char *s1, const char **s2, char c);
int __stdcall strcasematch (const char *s1, const char *s2);
int __stdcall strncasematch (const char *s1, const char *s2, size_t n);
char *__stdcall strcasestr (const char *searchee, const char *lookfor);

/* Time related */
void __stdcall totimeval (struct timeval *dst, FILETIME * src, int sub, int flag);
long __stdcall to_time_t (FILETIME * ptr);

/* pinfo table manipulation */
#ifndef lock_pinfo_for_update
int __stdcall lock_pinfo_for_update (DWORD timeout);
#endif
void unlock_pinfo (void);
void _stdcall set_myself (pid_t pid);

/* Retrieve a security descriptor that allows all access */
SECURITY_DESCRIPTOR *__stdcall get_null_sd (void);

int __stdcall get_id_from_sid (PSID, BOOL);
extern inline int get_uid_from_sid (PSID psid) { return get_id_from_sid (psid, FALSE);}
extern inline int get_gid_from_sid (PSID psid) { return get_id_from_sid (psid, TRUE); }

int __stdcall NTReadEA (const char *file, const char *attrname, char *buf, int len);
BOOL __stdcall NTWriteEA (const char *file, const char *attrname, char *buf, int len);

void __stdcall set_console_title (char *);
void set_console_handler ();

void __stdcall fill_rusage (struct rusage *, HANDLE);
void __stdcall add_rusage (struct rusage *, struct rusage *);

void set_winsock_errno ();

/**************************** Exports ******************************/

extern "C" {
int cygwin_select (int , fd_set *, fd_set *, fd_set *,
		   struct timeval *to);
int cygwin_gethostname (char *__name, size_t __len);

int kill_pgrp (pid_t, int);
int _kill (int, int);
int _raise (int sig);

int getfdtabsize ();
void setfdtabsize (int);

extern DWORD binmode;
extern char _data_start__, _data_end__, _bss_start__, _bss_end__;
extern void (*__CTOR_LIST__) (void);
extern void (*__DTOR_LIST__) (void);
};

/*************************** Unsorted ******************************/

#define WM_ASYNCIO	0x8000		// WM_APP

/* Note that MAX_PATH is defined in the windows headers */
/* There is also PATH_MAX and MAXPATHLEN.
   PATH_MAX is from Posix and does *not* include the trailing NUL.
   MAXPATHLEN is from Unix.

   Thou shalt use MAX_PATH throughout.  It avoids the NUL vs no-NUL
   issue and is neither of the Unixy ones [so we can punt on which
   one is the right one to use].  */

/* Initial and increment values for cygwin's fd table */
#define NOFILE_INCR    32

#include <sys/reent.h>

#define STD_RBITS (S_IRUSR | S_IRGRP | S_IROTH)
#define STD_WBITS (S_IWUSR)
#define STD_XBITS (S_IXUSR | S_IXGRP | S_IXOTH)

#define O_NOSYMLINK 0x080000
#define O_DIROPEN   0x100000

/*************************** Environment ******************************/

/* The structure below is used to control conversion to/from posix-style
 * file specs.  Currently, only PATH and HOME are converted, but PATH
 * needs to use a "convert path list" function while HOME needs a simple
 * "convert to posix/win32".  For the simple case, where a calculated length
 * is required, just return MAX_PATH.  *FIXME*
 */
struct win_env
  {
    const char *name;
    size_t namelen;
    char *posix;
    char *native;
    int (*toposix) (const char *, char *);
    int (*towin32) (const char *, char *);
    int (*posix_len) (const char *);
    int (*win32_len) (const char *);
    void add_cache (const char *in_posix, const char *in_native = NULL);
    const char * get_native () {return native ? native + namelen : NULL;}
  };

win_env * __stdcall getwinenv (const char *name, const char *posix = NULL);

void __stdcall update_envptrs ();
char * __stdcall winenv (const char * const *, int);
extern char **__cygwin_environ, ***main_environ;
extern "C" char __stdcall **cur_environ ();

/* The title on program start. */
extern char *old_title;
extern BOOL display_title;

#endif /* defined __cplusplus */
