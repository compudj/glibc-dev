/* Restartable Sequences exported symbols. Linux header.
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

#ifndef _SYS_RSEQ_H
#define _SYS_RSEQ_H	1

/* We use the structures declarations from the kernel headers.  */
#include <linux/rseq.h>
/* Architecture-specific rseq signature.  */
#include <bits/rseq.h>
#include <stdint.h>

extern __thread struct rseq __rseq_abi
__attribute__ ((tls_model ("initial-exec")));

/* Advertise Restartable Sequences registration ownership across
   application and shared libraries.

   Libraries and applications must check whether this variable is zero or
   non-zero if they wish to perform rseq registration on their own. If it
   is zero, it means restartable sequence registration is not handled, and
   the library or application is free to perform rseq registration. In
   that case, the library or application is taking ownership of rseq
   registration, and may set __rseq_handled to 1. It may then set it back
   to 0 after it completes unregistering rseq.

   If __rseq_handled is found to be non-zero, it means that another
   library (or the application) is currently handling rseq registration.

   Typical use of __rseq_handled is within library constructors and
   destructors, or at program startup.

   The fact that a library handles rseq registration is orthogonal to whether
   the running kernel implements the rseq system call or not.  */

extern int __rseq_handled;

#endif /* sys/rseq.h */
