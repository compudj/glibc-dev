/* Restartable Sequences exported symbols. Linux Implementation.
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

#include <sys/syscall.h>
#include <stdint.h>

#ifdef __NR_rseq
#include <sys/rseq.h>
#else

enum rseq_cpu_id_state {
  RSEQ_CPU_ID_UNINITIALIZED = -1,
  RSEQ_CPU_ID_REGISTRATION_FAILED = -2,
};

/* linux/rseq.h defines struct rseq as aligned on 32 bytes. The kernel ABI
   size is 20 bytes.  */
struct rseq {
  uint32_t cpu_id_start;
  uint32_t cpu_id;
  uint64_t rseq_cs;
  uint32_t flags;
} __attribute__ ((aligned(4 * sizeof(uint64_t))));

#endif

__thread struct rseq __rseq_abi = {
  .cpu_id = RSEQ_CPU_ID_UNINITIALIZED,
};
