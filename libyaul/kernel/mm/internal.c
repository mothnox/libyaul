/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <sys/cdefs.h>

#include <mm/tlsf.h>

#include <internal.h>

#define TLSF_HANDLE_PRIVATE       (0)
#define TLSF_HANDLE_USER          (1)
#define TLSF_HANDLE_COUNT         (2)

#define TLSF_POOL_PRIVATE_START ((uint32_t)&_private_pool[0])
#define TLSF_POOL_PRIVATE_SIZE  (0xA000)

#define TLSF_POOL_USER_START    ((uint32_t)&_end)
#define TLSF_POOL_USER_END      (HWRAM(HWRAM_SIZE))
#define TLSF_POOL_USER_SIZE     (TLSF_POOL_USER_END - TLSF_POOL_USER_START)

static uint8_t _private_pool[TLSF_POOL_PRIVATE_SIZE];

static tlsf_t _handles[TLSF_HANDLE_COUNT];

void
_internal_mm_init(void)
{
        master_state()->tlsf_handles = &_handles[0];

        master_state()->tlsf_handles[TLSF_HANDLE_PRIVATE] =
            tlsf_pool_create((void *)TLSF_POOL_PRIVATE_START, TLSF_POOL_PRIVATE_SIZE);

#if defined(MALLOC_IMPL_TLSF)
        master_state()->tlsf_handles[TLSF_HANDLE_USER] =
            tlsf_pool_create((void *)TLSF_POOL_USER_START, TLSF_POOL_USER_SIZE);
#else
        master_state()->tlsf_pools[TLSF_HANDLE_USER] = NULL;
#endif /* MALLOC_IMPL_TLSF */
}

void *
_internal_malloc(size_t n)
{
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_PRIVATE];

        void *ret = tlsf_malloc(handle, n);

        MEMORY_WRITE(32, LWRAM(0), tlsf_check(handle));
        MEMORY_WRITE(32, LWRAM(4), tlsf_pool_check(tlsf_pool_get(handle)));

        return ret;
}

void *
_internal_realloc(void *old, size_t new_len)
{
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_PRIVATE];

        void * ret = tlsf_realloc(handle, old, new_len);

        MEMORY_WRITE(32, LWRAM(8), tlsf_check(handle));
        MEMORY_WRITE(32, LWRAM(12), tlsf_pool_check(tlsf_pool_get(handle)));

        return ret;
}

void *
_internal_memalign(size_t n, size_t align)
{
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_PRIVATE];

        void *ret = tlsf_memalign(handle, n, align);

        MEMORY_WRITE(32, LWRAM(16), tlsf_check(handle));
        MEMORY_WRITE(32, LWRAM(20), tlsf_pool_check(tlsf_pool_get(handle)));

        return ret;
}

void
_internal_free(void *addr)
{
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_PRIVATE];

        tlsf_free(handle, addr);

        MEMORY_WRITE(32, LWRAM(24), tlsf_check(handle));
        MEMORY_WRITE(32, LWRAM(28), tlsf_pool_check(tlsf_pool_get(handle)));

        /* MEMORY_WRITE(32, LWRAM(16), tlsf_size()); */
        /* MEMORY_WRITE(32, LWRAM(20), tlsf_pool_overhead()); */
        /* MEMORY_WRITE(32, LWRAM(24), tlsf_alloc_overhead()); */
}

void *
_internal_user_malloc(size_t n __unused) /* Keep as __unused */
{
#if defined(MALLOC_IMPL_TLSF)
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_USER];

        return tlsf_malloc(handle, n);
#else
        assert(false && "Missing implementation. Override malloc symbol");
#endif /* MALLOC_IMPL_TLSF */
}

void *
_internal_user_realloc(void *old __unused, size_t new_len __unused) /* Keep as __unused */
{
#if defined(MALLOC_IMPL_TLSF)
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_USER];

        return tlsf_realloc(handle, old, new_len);
#else
        assert(false && "Missing implementation. Override realloc symbol");
#endif /* MALLOC_IMPL_TLSF */
}

void *
_internal_user_memalign(size_t n __unused, size_t align __unused) /* Keep as __unused */
{
#if defined(MALLOC_IMPL_TLSF)
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_USER];

        return tlsf_memalign(handle, align, n);
#else
        assert(false && "Missing implementation. Override memalign symbol");
#endif /* MALLOC_IMPL_TLSF */
}

void
_internal_user_free(void *addr __unused) /* Keep as __unused */
{
#if defined(MALLOC_IMPL_TLSF)
        tlsf_t const handle = master_state()->tlsf_handles[TLSF_HANDLE_USER];

        tlsf_free(handle, addr);
#else
        assert(false && "Missing implementation. Override malloc symbol");
#endif /* MALLOC_IMPL_TLSF */
}
