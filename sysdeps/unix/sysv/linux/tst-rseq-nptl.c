/* Restartable Sequences NPTL test.

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

/* These tests validate that rseq is registered from various execution
   contexts (main thread, constructor, destructor, other threads, other
   threads created from constructor and destructor, forked process
   (without exec), pthread_atfork handlers, pthread setspecific
   destructors, C++ thread and process destructors, signal handlers,
   atexit handlers).

   See the Linux kernel selftests for extensive rseq stress-tests.  */

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <support/check.h>
#include <support/xthread.h>

#ifdef __NR_rseq
#include <sys/rseq.h>
#endif

#if defined __NR_rseq && defined RSEQ_SIG
#include <pthread.h>
#include <syscall.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <atomic.h>

static pthread_key_t rseq_test_key;

static int
rseq_thread_registered (void)
{
  return (int32_t) atomic_load_relaxed (&__rseq_abi.cpu_id) >= 0;
}

static int
do_rseq_main_test (void)
{
  if (raise (SIGUSR1))
    FAIL_EXIT1 ("error raising signal");
  if (pthread_setspecific (rseq_test_key, (void *) 1l))
    FAIL_EXIT1 ("error in pthread_setspecific");
  if (!rseq_thread_registered ())
    {
      FAIL_RET ("rseq not registered in main thread");
    }
  return 0;
}

#if 0
static void
cancel_routine (void *arg)
{
  if (!rseq_thread_registered ())
    {
      printf ("rseq not registered in cancel routine\n");
      support_record_failure ();
    }
}

static int cancel_thread_ready;

static void
test_cancel_thread (void)
{
  pthread_cleanup_push (cancel_routine, NULL);
  atomic_store_release (&cancel_thread_ready, 1);
  for (;;)
    usleep (100);
  pthread_cleanup_pop (0);
}
#endif

static void *
thread_function (void * arg)
{
  //int i = (int) (intptr_t) arg;

  if (raise (SIGUSR1))
    FAIL_EXIT1 ("error raising signal");
#if 0
  if (i == 0)
    test_cancel_thread ();
#endif
  if (pthread_setspecific (rseq_test_key, (void *) 1l))
    FAIL_EXIT1 ("error in pthread_setspecific");
  return rseq_thread_registered () ? NULL : (void *) 1l;
}

static void
sighandler (int sig)
{
  if (!rseq_thread_registered ())
    {
      printf ("rseq not registered in signal handler\n");
      support_record_failure ();
    }
}

static void
setup_signals (void)
{
  struct sigaction sa;

  sigemptyset (&sa.sa_mask);
  sigaddset (&sa.sa_mask, SIGUSR1);
  sa.sa_flags = 0;
  sa.sa_handler = sighandler;
  if (sigaction (SIGUSR1, &sa, NULL) != 0)
    {
      FAIL_EXIT1 ("sigaction failure: %s", strerror (errno));
    }
}

#define N 7
static const int t[N] = { 1, 2, 6, 5, 4, 3, 50 };

static int
do_rseq_threads_test (int nr_threads)
{
  pthread_t th[nr_threads];
  int i;
  int result = 0;

  //cancel_thread_ready = 0;
  for (i = 0; i < nr_threads; ++i)
    if (pthread_create (&th[i], NULL, thread_function,
                        (void *) (intptr_t) i) != 0)
      {
        FAIL_EXIT1 ("creation of thread %d failed", i);
      }

#if 0
  while (!atomic_load_acquire (&cancel_thread_ready))
    usleep (100);

  if (pthread_cancel (th[0]))
    FAIL_EXIT1 ("error in pthread_cancel");
#endif

  for (i = 0; i < nr_threads; ++i)
    {
      void *v;
      if (pthread_join (th[i], &v) != 0)
        {
          printf ("join of thread %d failed\n", i);
          result = 1;
        }
#if 0
      else if (i != 0 && v != NULL)
        {
          printf ("join %d successful, but child failed\n", i);
          result = 1;
        }
      else if (i == 0 && v == NULL)
        {
          printf ("join %d successful, child did not fail as expected\n", i);
          result = 1;
        }
#else
      else if (v != NULL)
        {
          printf ("join %d successful, but child failed\n", i);
          result = 1;
        }
#endif
    }
  return result;
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
do_rseq_fork_test (void)
{
  int status;
  pid_t pid, retpid;

  pid = fork ();
  switch (pid)
    {
      case 0:
        exit (do_rseq_main_test ());
      case -1:
        FAIL_EXIT1 ("Unexpected fork error %s", strerror (errno));
    }
  retpid = TEMP_FAILURE_RETRY (waitpid (pid, &status, 0));
  if (retpid != pid)
    {
      FAIL_EXIT1 ("waitpid returned %ld, expected %ld",
                  (long int) retpid, (long int) pid);
    }
  if (WEXITSTATUS (status))
    {
      printf ("rseq not registered in child\n");
      return 1;
    }
  return 0;
}

static int
do_rseq_test (void)
{
  int i, result = 0;

  if (!rseq_available ())
    {
      FAIL_UNSUPPORTED ("kernel does not support rseq, skipping test");
    }
  setup_signals ();
  if (raise (SIGUSR1))
    FAIL_EXIT1 ("error raising signal");
  if (do_rseq_main_test ())
    result = 1;
  for (i = 0; i < N; i++)
    {
      if (do_rseq_threads_test (t[i]))
        result = 1;
    }
  if (do_rseq_fork_test ())
    result = 1;
  return result;
}

static void
atfork_prepare (void)
{
  if (!rseq_thread_registered ())
    {
      printf ("rseq not registered in pthread atfork prepare\n");
      support_record_failure ();
    }
}

static void
atfork_parent (void)
{
  if (!rseq_thread_registered ())
    {
      printf ("rseq not registered in pthread atfork parent\n");
      support_record_failure ();
    }
}

static void
atfork_child (void)
{
  if (!rseq_thread_registered ())
    {
      printf ("rseq not registered in pthread atfork child\n");
      support_record_failure ();
    }
}

static void
rseq_key_destructor (void *arg)
{
  /* Cannot use deferred failure reporting after main () returns.  */
  if (!rseq_thread_registered ())
    FAIL_EXIT1 ("rseq not registered in pthread key destructor");
}

static void
atexit_handler (void)
{
  /* Cannot use deferred failure reporting after main () returns.  */
  if (!rseq_thread_registered ())
    FAIL_EXIT1 ("rseq not registered in atexit handler");
}

static void __attribute__ ((constructor))
do_rseq_constructor_test (void)
{
  support_record_failure_init ();
  if (!__rseq_handled)
    FAIL_EXIT1 ("rseq registration ownership is not published by libc");
  if (atexit (atexit_handler))
    FAIL_EXIT1 ("error calling atexit");
  xpthread_key_create (&rseq_test_key, rseq_key_destructor);
  if (pthread_atfork (atfork_prepare, atfork_parent, atfork_child))
    FAIL_EXIT1 ("error calling pthread_atfork");
  if (do_rseq_test ())
    FAIL_EXIT1 ("rseq not registered within constructor");
}

static void __attribute__ ((destructor))
do_rseq_destructor_test (void)
{
  /* Cannot use deferred failure reporting after main () returns.  */
  if (do_rseq_test ())
    FAIL_EXIT1 ("rseq not registered within destructor");
  xpthread_key_delete (rseq_test_key);
}

/* Test C++ destructor called at thread and process exit.  */
void
__call_tls_dtors (void)
{
  /* Cannot use deferred failure reporting after main () returns.  */
  if (!rseq_thread_registered ())
    FAIL_EXIT1 ("rseq not registered in C++ thread/process exit destructor");
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

static int
do_test (void)
{
  if (!__rseq_handled)
    FAIL_EXIT1 ("rseq registration ownership is not published by libc");
  return do_rseq_test ();
}

#include <support/test-driver.c>
