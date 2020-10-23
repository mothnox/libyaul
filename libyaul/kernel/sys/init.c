/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 * Joe Fenton <jlfenton65@gmail.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/cdefs.h>

#include <cpu/cache.h>

#include <internal.h>

void __weak
user_init(void)
{
}

static void __used
_call_global_ctors(void)
{
        extern void (*__CTOR_LIST__[])(void);

        /* Constructors are called in reverse order of the list */
        int32_t i;
        for (i = (int32_t)__CTOR_LIST__[0]; i >= 1; i--) {
                /* Each function handles one or more destructor (within
                 * file scope) */
                __CTOR_LIST__[i]();
        }
}

/* Do all destructors */
static void __used
_call_global_dtors(void)
{
        extern void (*__DTOR_LIST__[])(void);

        /* Destructors in forward order */
        int32_t i;
        for (i = 0; i < (int32_t)__DTOR_LIST__[0]; i++) {
                /* Each function handles one or more destructor (within
                 * file scope) */
                __DTOR_LIST__[i + 1]();
        }
}

static void __used __section(".init")
_init(void)
{
        _internal_mm_init();
        
        _call_global_ctors();

        _internal_cpu_init();
        _internal_scu_init();
        _internal_smpc_init();
        _internal_smpc_peripheral_init();

#if HAVE_DEV_CARTRIDGE == 1 /* USB flash cartridge */
        _internal_usb_cart_init();
#else
        _internal_dram_cart_init();
#endif /* HAVE_DEV_CARTRIDGE */

        _internal_dma_queue_init();

        _internal_vdp_init();
        _internal_dbgio_init();

        user_init();

        cpu_cache_purge();
}

static void __section(".fini") __used __noreturn
_fini(void)
{
        _call_global_dtors();

        while (true) {
        }

        __builtin_unreachable();
}
