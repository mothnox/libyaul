/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _SMPC_SMC_H_
#define _SMPC_SMC_H_

#include <sys/cdefs.h>

#include <stdint.h>

#include <smpc/map.h>

__BEGIN_DECLS

/// @defgroup CPU_SMPC_SMC_DEFINES
/// @defgroup CPU_SMPC_SMC_INLINE_FUNCTIONS

/// @addtogroup CPU_SMPC_SMC_DEFINES
/// @{

/// Not yet documented.
#define SMPC_SMC_MSHON          0x00
/// Not yet documented.
#define SMPC_SMC_SSHON          0x02
/// Not yet documented.
#define SMPC_SMC_SSHOFF         0x03
/// Not yet documented.
#define SMPC_SMC_SNDON          0x06
/// Not yet documented.
#define SMPC_SMC_SNDOFF         0x07
/// Not yet documented.
#define SMPC_SMC_CDON           0x08
/// Not yet documented.
#define SMPC_SMC_CDOFF          0x09
/// Not yet documented.
#define SMPC_SMC_SYSRES         0x0D
/// Not yet documented.
#define SMPC_SMC_CKCHG352       0x0E
/// Not yet documented.
#define SMPC_SMC_CKCHG320       0x0F
/// Not yet documented.
#define SMPC_SMC_NMIREQ         0x18
/// Not yet documented.
#define SMPC_SMC_RESENAB        0x19
/// Not yet documented.
#define SMPC_SMC_RESDISA        0x1A
/// Not yet documented.
#define SMPC_SMC_INTBACK        0x10
/// Not yet documented.
#define SMPC_SMC_SETSMEM        0x17
/// Not yet documented.
#define SMPC_RTC_SETTIME        0x16

/// @}

/// @addtogroup CPU_SMPC_SMC_INLINE_FUNCTIONS
/// @{

/// @brief Not yet documented.
static inline void __always_inline
smpc_smc_wait(bool enter)
{
        volatile uint8_t *reg_sf;
        reg_sf = (volatile uint8_t *)SMPC(SF);

        while ((*reg_sf & 0x01) == 0x01);

        if (enter) {
                *reg_sf = 0x01;
        }
}

/// @brief Not yet documented.
static inline uint8_t __always_inline
smpc_smc_call(uint8_t cmd)
{
        smpc_smc_wait(true);

        MEMORY_WRITE(8, SMPC(COMREG), cmd);

        smpc_smc_wait(false);

        return MEMORY_READ(8, OREG(31));
}

/// @brief Not yet documented.
static inline uint8_t __always_inline
smpc_smc_cdoff_call(void)
{
        return smpc_smc_call(SMPC_SMC_CDOFF);
}

/// @brief Not yet documented.
static inline uint8_t __always_inline
smpc_smc_cdon_call(void)
{
        return smpc_smc_call(SMPC_SMC_CDON);
}

/// @brief Not yet documented.
static inline uint8_t __always_inline
smpc_smc_ckchg320_call(void)
{
        return smpc_smc_call(SMPC_SMC_CKCHG320);
}

/// @brief Not yet documented.
static inline uint8_t __always_inline
smpc_smc_ckchg352_call(void)
{
        return smpc_smc_call(SMPC_SMC_CKCHG352);
}

/// @brief Not yet documented.
static inline uint8_t __always_inline
smpc_smc_intback_call(uint8_t ireg0, uint8_t ireg1)
{
        /* Fetch peripheral data "SMPC" status */
        smpc_smc_wait(true);

        MEMORY_WRITE(8, IREG(0), ireg0 & 0x01);
        MEMORY_WRITE(8, IREG(1), ireg1 & 0x7F);
        MEMORY_WRITE(8, IREG(2), 0xF0);

        MEMORY_WRITE(8, SMPC(COMREG), SMPC_SMC_INTBACK);

        return 0;
}

static inline uint8_t __always_inline
smpc_smc_mshon_call(void)
{
        return smpc_smc_call(SMPC_SMC_MSHON);
}

static inline uint8_t __always_inline
smpc_smc_nmireq_call(void)
{
        return smpc_smc_call(SMPC_SMC_NMIREQ);
}

static inline uint8_t __always_inline
smpc_smc_resdisa_call(void)
{
        return smpc_smc_call(SMPC_SMC_RESDISA);
}

static inline uint8_t __always_inline
smpc_smc_resenab_call(void)
{
        return smpc_smc_call(SMPC_SMC_RESENAB);
}

static inline uint8_t __always_inline
smpc_smc_setsmem_call(void)
{
        return smpc_smc_call(SMPC_SMC_SETSMEM);
}

static inline uint8_t __always_inline
smpc_smc_sndoff_call(void)
{
        return smpc_smc_call(SMPC_SMC_SNDOFF);
}

static inline uint8_t __always_inline
smpc_smc_sndon_call(void)
{
        return smpc_smc_call(SMPC_SMC_SNDON);
}

static inline uint8_t __always_inline
smpc_smc_sshoff_call(void)
{
        return smpc_smc_call(SMPC_SMC_SSHOFF);
}

static inline uint8_t __always_inline
smpc_smc_sshon_call(void)
{
        return smpc_smc_call(SMPC_SMC_SSHON);
}

static inline void __always_inline __noreturn
smpc_smc_sysres_call(void)
{
        smpc_smc_wait(true);

        MEMORY_WRITE(8, SMPC(COMREG), SMPC_SMC_SYSRES);

        smpc_smc_wait(false);

        __builtin_unreachable();
}

/// @}

__END_DECLS

#endif /* !_SMPC_SMC_H_ */
