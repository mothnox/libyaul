/*
 * Copyright (c) 2012-2019
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com
 */

#include <sys/cdefs.h>

#include <stdio.h>

#include <bios-internal.h>

#include <cpu/intc.h>
#include <cpu/map.h>
#include <cpu/wdt.h>

static void _master_iti_handler(void);
static void _slave_iti_handler(void);

static void _iti_handler(cpu_wdt_ihr_t iti_ihr);

static cpu_wdt_ihr_t _iti_ihr_table[] = {
        __BIOS_DEFAULT_HANDLER,
        __BIOS_DEFAULT_HANDLER
};

static cpu_wdt_ihr_t *_iti_ihr_get(void);

void
cpu_wdt_init(cpu_wdt_clock_t clock_div)
{
        MEMORY_WRITE_AND(16, CPU(VCRWDT), ~0x7F00);
        MEMORY_WRITE_OR(16, CPU(VCRWDT), CPU_INTC_INTERRUPT_WDT_ITI << 8);

        MEMORY_WRITE_WTCSR(clock_div & 0x07);

        MEMORY_CLEAR_WOVF_RSTCSR();
        MEMORY_CLEAR_RSTCSR(0x00);

        const cpu_which_t which_cpu = cpu_dual_executor_get();

        if (which_cpu == CPU_MASTER) {
                cpu_intc_ihr_set(CPU_INTC_INTERRUPT_WDT_ITI, _master_iti_handler);
                cpu_intc_ihr_set(CPU_INTC_INTERRUPT_WDT_ITI + CPU_INTC_INTERRUPT_SLAVE_BASE,
                    _slave_iti_handler);
        }
}

void
cpu_wdt_timer_mode_set(cpu_wdt_mode_t mode, cpu_wdt_ihr_t ihr)
{
        uint8_t wtcr_bits = MEMORY_READ(8, CPU(WTCSRR));

        /* Clear OVF and TME bits */
        wtcr_bits &= ~0xA0;
        /* Set WTIT bit (timer mode) if necessary */
        wtcr_bits |= (mode & 0x1) << 4;

        MEMORY_WRITE_WTCSR(wtcr_bits);

        MEMORY_CLEAR_WOVF_RSTCSR();
        MEMORY_CLEAR_RSTCSR(0x00);

        cpu_wdt_ihr_t * const iti_ihr = _iti_ihr_get();

        *iti_ihr = __BIOS_DEFAULT_HANDLER;

        if (ihr != NULL) {
                *iti_ihr = ihr;
        }

        cpu_wdt_count_set(0);
}

static void __interrupt_handler
_master_iti_handler(void)
{
        _iti_handler(_iti_ihr_table[CPU_MASTER]);
}

static void __interrupt_handler
_slave_iti_handler(void)
{
        _iti_handler(_iti_ihr_table[CPU_SLAVE]);
}

static void
_iti_handler(cpu_wdt_ihr_t iti_ihr)
{
        uint8_t wtcr_bits = MEMORY_READ(8, CPU(WTCSRR));

        /* Clear OVF bit */
        wtcr_bits &= ~0x80;

        MEMORY_WRITE_WTCSR(wtcr_bits);

        /* Reset RSTE bit when WTCNT overflows */
        /* MEMORY_CLEAR_RSTCSR(0x40); */

        /* User is responsible for resetting WDT count */
        iti_ihr();
}

static cpu_wdt_ihr_t *
_iti_ihr_get(void)
{
        const cpu_which_t which_cpu = cpu_dual_executor_get();

        return &_iti_ihr_table[which_cpu];
}
