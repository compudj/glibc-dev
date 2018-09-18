/* Restartable Sequences internal API. Linux implementation.
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

#ifndef RSEQ_INTERNAL_H
#define RSEQ_INTERNAL_H

#include <sysdep.h>
#include <errno.h>

#ifdef __NR_rseq
#include <sys/rseq.h>
#endif

#if defined __NR_rseq && defined RSEQ_SIG

static inline int
rseq_register_current_thread (void)
{
  int rc, ret = 0;
  INTERNAL_SYSCALL_DECL (err);

  if (__rseq_abi.cpu_id == RSEQ_CPU_ID_REGISTRATION_FAILED)
    return -1;
  rc = INTERNAL_SYSCALL_CALL (rseq, err, &__rseq_abi, sizeof (struct rseq),
                              0, RSEQ_SIG);
  if (!rc)
    goto end;
  if (INTERNAL_SYSCALL_ERRNO (rc, err) != EBUSY)
    __rseq_abi.cpu_id = RSEQ_CPU_ID_REGISTRATION_FAILED;
  ret = -1;
end:
  return ret;
}

static inline int
rseq_unregister_current_thread (void)
{
  int rc, ret = 0;
  INTERNAL_SYSCALL_DECL (err);

  rc = INTERNAL_SYSCALL_CALL (rseq, err, &__rseq_abi, sizeof (struct rseq),
                              RSEQ_FLAG_UNREGISTER, RSEQ_SIG);
  if (!rc)
    goto end;
  ret = -1;
end:
  return ret;
}

static inline void
rseq_init (void)
{
  __rseq_handled = 1;
}
#else
static inline int
rseq_register_current_thread (void)
{
  return -1;
}

static inline int
rseq_unregister_current_thread (void)
{
  return -1;
}

static inline void
rseq_init (void)
{
}
#endif

#endif /* rseq-internal.h */
