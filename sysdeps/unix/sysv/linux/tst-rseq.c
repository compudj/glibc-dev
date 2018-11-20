/* Restartable Sequences single-threaded tests.

   Copyright (C) 2019 Free Software Foundation, Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* These tests validate that rseq is registered from main in an executable
   not linked against libpthread.  */

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <support/check.h>

#ifdef __NR_rseq
#include <sys/rseq.h>
#endif

#if defined __NR_rseq && defined RSEQ_SIG
#include <syscall.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <atomic.h>

static int
rseq_thread_registered (void)
{
  return (int32_t) atomic_load_relaxed (&__rseq_abi.cpu_id) >= 0;
}

static int
do_rseq_main_test (void)
{
  if (!rseq_thread_registered ())
    {
      FAIL_RET ("rseq not registered in main thread");
    }
  return 0;
}

static int
sys_rseq (struct rseq *rseq_abi, uint32_t rseq_len, int flags, uint32_t sig)
{
  return syscall (__NR_rseq, rseq_abi, rseq_len, flags, sig);
}

static int
rseq_available (void)
{
  int rc;

  rc = sys_rseq (NULL, 0, 0, 0);
  if (rc != -1)
    FAIL_EXIT1 ("Unexpected rseq return value %d", rc);
  switch (errno)
    {
    case ENOSYS:
      return 0;
    case EINVAL:
      return 1;
    default:
      FAIL_EXIT1 ("Unexpected rseq error %s", strerror (errno));
    }
}

static int
do_rseq_test (void)
{
  int result = 0;

  if (!rseq_available ())
    {
      FAIL_UNSUPPORTED ("kernel does not support rseq, skipping test");
    }
  if (do_rseq_main_test ())
    result = 1;
  return result;
}
#else
static int
do_rseq_test (void)
{
#ifndef __NR_rseq
  FAIL_UNSUPPORTED ("kernel headers do not support rseq, skipping test");
#endif
#ifndef RSEQ_SIG
  FAIL_UNSUPPORTED ("glibc does not define RSEQ_SIG, skipping test");
#endif
  return 0;
}
#endif

/* Test whether glibc correctly sets __rseq_handled ownership by
   providing an instance of the symbol within the executable.
   The dynamic linker ensures all libraries and the executable
   agree on choosing a single copy of this global symbol.  */
int __rseq_handled;

static int
do_test (void)
{
  if (!__rseq_handled)
    FAIL_RET ("rseq registration ownership is not published by libc");
  return do_rseq_test ();
}

#include <support/test-driver.c>
