/*
 * System call stubs for ARM GCC 12 compatibility
 *
 * Note: sl_memory.c from Gecko SDK provides heap management,
 * but we need additional system call stubs for ARM GCC 12.
 */

#include <sys/types.h>
#include <errno.h>

/*---------------------------------------------------------------------------*/
int
_close(int fd)
{
  (void)fd;
  return -1;
}
/*---------------------------------------------------------------------------*/
int
_fstat(int fd, void *st)
{
  (void)fd;
  (void)st;
  return -1;
}
/*---------------------------------------------------------------------------*/
int
_getpid(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
int
_isatty(int fd)
{
  (void)fd;
  return 0;
}
/*---------------------------------------------------------------------------*/
int
_kill(int pid, int sig)
{
  (void)pid;
  (void)sig;
  return -1;
}
/*---------------------------------------------------------------------------*/
int
_lseek(int fd, int offset, int whence)
{
  (void)fd;
  (void)offset;
  (void)whence;
  return -1;
}
/*---------------------------------------------------------------------------*/
int
_read(int fd, char *buf, int count)
{
  (void)fd;
  (void)buf;
  (void)count;
  return -1;
}
/*---------------------------------------------------------------------------*/
int
_write(int fd, const char *buf, int count)
{
  (void)fd;
  (void)buf;
  (void)count;
  return -1;
}
/*---------------------------------------------------------------------------*/
