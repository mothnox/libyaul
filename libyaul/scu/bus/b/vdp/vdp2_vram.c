/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <string.h>

#include <vdp2/cram.h>
#include <vdp2/vram.h>

#include "vdp-internal.h"

void
vdp2_vram_control_set(const struct vdp2_vram_ctl *vram_ctl)
{
#ifdef DEBUG
        assert(vram_ctl != NULL);

        assert((vram_ctl->vram_mode & 0xFC) == 0x00);

        assert((vram_ctl->coefficient_table == VDP2_VRAM_CTL_COEFFICIENT_TABLE_VRAM) ||
              ((vram_ctl->coefficient_table == VDP2_VRAM_CTL_COEFFICIENT_TABLE_CRAM)));
#endif /* DEBUG */

        /* If the coefficient table is set to be stored in CRAM, the
         * color mode must be 1 */
        if (vram_ctl->coefficient_table == VDP2_VRAM_CTL_COEFFICIENT_TABLE_CRAM) {
                _state_vdp2()->regs.ramctl |= 0x1000;
        }

        /* Coefficient table storage */
        _state_vdp2()->regs.ramctl &= 0x7FFF;
        _state_vdp2()->regs.ramctl |= vram_ctl->coefficient_table << 15;

        /* VRAM mode */
        _state_vdp2()->regs.ramctl &= 0xFCFF;
        _state_vdp2()->regs.ramctl |= vram_ctl->vram_mode << 8;

        MEMORY_WRITE(16, VDP2(VRSIZE), 0x0000);
        MEMORY_WRITE(16, VDP2(RAMCTL), _state_vdp2()->regs.ramctl);

        (void)memcpy(&_state_vdp2()->vram_ctl, &vram_ctl,
            sizeof(struct vdp2_vram_ctl));
}

void
vdp2_vram_cycp_set(const struct vdp2_vram_cycp *vram_cycp)
{
#ifdef DEBUG
        assert(vram_cycp != NULL);
#endif /* DEBUG */

        vdp2_vram_cycp_bank_set(0, &vram_cycp->pt[0]);
        vdp2_vram_cycp_bank_set(1, &vram_cycp->pt[1]);
        vdp2_vram_cycp_bank_set(2, &vram_cycp->pt[2]);
        vdp2_vram_cycp_bank_set(3, &vram_cycp->pt[3]);
}

void
vdp2_vram_cycp_clear(void)
{
        vdp2_vram_cycp_bank_clear(0);
        vdp2_vram_cycp_bank_clear(1);
        vdp2_vram_cycp_bank_clear(2);
        vdp2_vram_cycp_bank_clear(3);
}

struct vdp2_vram_cycp_bank
vdp2_vram_cycp_bank_get(uint8_t bank)
{
#ifdef DEBUG
        assert((bank >= 0) && (bank <= 3));
#endif /* DEBUG */

        return *(struct vdp2_vram_cycp_bank *)&_state_vdp2()->regs.cyc[bank];
}

void
vdp2_vram_cycp_bank_set(uint8_t bank,
    const struct vdp2_vram_cycp_bank *cycp_bank)
{
#ifdef DEBUG
        assert((bank >= 0) && (bank <= 3));

        assert(cycp_bank != NULL);
#endif /* DEBUG */

        _state_vdp2()->regs.cyc[bank] = cycp_bank->raw;
}

void
vdp2_vram_cycp_bank_clear(uint8_t bank)
{
#ifdef DEBUG
        assert((bank >= 0) && (bank <= 3));
#endif /* DEBUG */

        _state_vdp2()->regs.cyc[bank] = 0xFFFFFFFF;
}
