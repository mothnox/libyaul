#ifndef _YAUL_KERNEL_MM_TLSF_H_
#define _YAUL_KERNEL_MM_TLSF_H_

/* Two Level Segregated Fit memory allocator, version 3.1.
 *
 * Written by Matthew Conte
 *      http://tlsf.baisoku.org
 *
 * Based on the original documentation by Miguel Masmano:
 *      http://www.gii.upv.es/tlsf/main/docs
 *
 * This implementation was written to the specification
 * of the document, therefore no GPL restrictions apply.
 *
 * Copyright (c) 2006-2016, Matthew Conte
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL MATTHEW CONTE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <stddef.h>

__BEGIN_DECLS

/* tlsf_t: a TLSF structure. Can contain 1 to N pools. */
/* pool_t: a block of memory that TLSF can manage. */
typedef void *tlsf_t;
typedef void *tlsf_pool_t;

/* Create/destroy a memory pool. */
extern tlsf_t tlsf_create(void *mem);
extern tlsf_t tlsf_pool_create(void *mem, size_t bytes);
extern void tlsf_destroy(tlsf_t tlsf);
extern tlsf_pool_t tlsf_pool_get(tlsf_t tlsf);

/* Add/remove memory pools. */
extern tlsf_pool_t tlsf_pool_add(tlsf_t tlsf, void *mem, size_t bytes);
extern void tlsf_pool_remove(tlsf_t tlsf, tlsf_pool_t pool);

/* malloc/memalign/realloc/free replacements. */
extern void *tlsf_malloc(tlsf_t tlsf, size_t bytes);
extern void *tlsf_memalign(tlsf_t tlsf, size_t align, size_t bytes);
extern void *tlsf_realloc(tlsf_t tlsf, void *ptr, size_t size);
extern void tlsf_free(tlsf_t tlsf, void *ptr);

/* Returns internal block size, not original request size */
extern size_t tlsf_block_size(void *ptr);

/* Overheads/limits of internal structures. */
extern size_t tlsf_size(void);
extern size_t tlsf_align_size(void);
extern size_t tlsf_block_size_min(void);
extern size_t tlsf_block_size_max(void);
extern size_t tlsf_pool_overhead(void);
extern size_t tlsf_alloc_overhead(void);

/* Debugging. */
typedef void (*tlsf_walker_t)(void *ptr, size_t size, int used, void *user);

extern void tlsf_pool_walk(tlsf_pool_t pool, tlsf_walker_t walker, void *user);
/* Returns nonzero if any internal consistency check fails. */
extern int tlsf_check(tlsf_t tlsf);
extern int tlsf_pool_check(tlsf_pool_t pool);

__END_DECLS

#endif /* !_YAUL_KERNEL_MM_TLSF_H_ */
