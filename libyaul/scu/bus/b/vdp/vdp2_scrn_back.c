/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>

#include <vdp.h>

#include "vdp-internal.h"

static void _back_set(vdp2_vram_t vram, const rgb1555_t *buffer, uint32_t count);

void
vdp2_scrn_back_color_set(vdp2_vram_t vram, rgb1555_t color)
{
        static rgb1555_t buffer = RGB1555_INITIALIZER(1, 0, 0, 0);

        buffer = color;

        _back_set(vram, &buffer, 1);
}

void
vdp2_scrn_back_buffer_set(vdp2_vram_t vram, const rgb1555_t *buffer,
    uint32_t count)
{
        assert(buffer != NULL);
        assert(count > 0);

        _back_set(vram, buffer, count);
}

void
vdp2_scrn_back_sync(void)
{
        const size_t len = _state_vdp2()->back.len;

        if (len == 0) {
                return;
        }

        vdp_dma_enqueue((void *)_state_vdp2()->back.vram,
            _state_vdp2()->back.buffer,
            len);
}

static void
_back_set(vdp2_vram_t vram, const rgb1555_t *buffer, uint32_t count)
{
        /* If set to single color, transfer only one color value. Otherwise,
         * transfer a color buffer */
        const uint16_t bkclmd = (count == 1) ? 0x0000 : 0x8000;

        _state_vdp2()->regs->bktau = bkclmd | VDP2_VRAM_BANK(vram);
        _state_vdp2()->regs->bktal = (vram >> 1) & 0xFFFF;

        if (count == 1) {
                MEMORY_WRITE(16, vram, buffer->raw);
        } else {
                _state_vdp2()->back.vram = vram;
                _state_vdp2()->back.buffer = buffer;
                _state_vdp2()->back.len = count * sizeof(rgb1555_t);
        }
}
