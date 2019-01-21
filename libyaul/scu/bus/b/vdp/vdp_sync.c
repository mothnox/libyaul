/*
 * Copyright (c) 2012-2018 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <cpu/cache.h>
#include <cpu/intc.h>

#include <scu/ic.h>

#include <vdp.h>

#include <sys/dma-queue.h>

#include "vdp-internal.h"

#define USER_CALLBACK_COUNT 16

#define SCU_MASK_ENABLE         (IC_MASK_VBLANK_IN | IC_MASK_VBLANK_OUT | IC_MASK_SPRITE_END)
#define SCU_MASK_DISABLE        (~(IC_MASK_VBLANK_IN | IC_MASK_VBLANK_OUT | IC_MASK_SPRITE_END) & IC_MASK_ALL)

#define STATE_NONE                      0x00

#define STATE_SYNC                      0x01
#define STATE_SYNC_VBLANK_IN            0x02
#define STATE_SYNC_VBLANK_OUT           0x04

#define STATE_VDP1_REQUEST_COMMIT_LIST  0x01 /* VDP1 Request for VDP1 to draw */
#define STATE_VDP1_LIST_TRANSFERRED     0x02 /* VDP1 finished transferring via DMA */
#define STATE_VDP1_LIST_COMMITTED       0x04 /* VDP1 finished drawing */
#define STATE_VDP1_REQUEST_CHANGE       0x08 /* VDP1 request to change frame buffer */

#define STATE_VDP2_REQUEST_COMMIT       0x01 /* VDP2 request to commit state */
#define STATE_VDP2_REQUEST_PENDING      0x02 /* VDP2 request commit is pending */
#define STATE_VDP2_COMMITTED            0x04 /* VDP2 finished updating state */

static void _vblank_in_handler(void);
static void _vblank_out_handler(void);
static void _sprite_end_handler(void);

static void _vdp1_dma_handler(const struct dma_queue_transfer *);

static void _vdp2_commit_handler(const struct dma_queue_transfer *);

static void _default_handler(void);

static inline bool __always_inline _interlace_mode_double(void);
static inline bool __always_inline _vdp1_transfer_over(void);

static volatile struct {
        uint8_t sync;
        uint8_t vdp1;
        uint8_t vdp2;
        uint8_t field_count;
} _state __aligned(4);

/* Keep track of the current command table operation */
static volatile uint16_t _vdp1_last_command = 0x0000;
/* SCU-DMA state */
static struct dma_level_cfg _vdp1_dma_cfg;
static struct dma_reg_buffer _vdp1_dma_reg_buffer;

static void (*_user_callbacks[USER_CALLBACK_COUNT])(void *);
static void *_user_works[USER_CALLBACK_COUNT];

static void (*_user_vblank_in_handler)(void);
static void (*_user_vblank_out_handler)(void);

static void _default_user_callback(void *);

static void _init_vdp1(void);
static void _init_vdp2(void);

void
vdp_sync_init(void)
{
        scu_ic_mask_chg(IC_MASK_ALL, SCU_MASK_ENABLE);

        _init_vdp1();
        _init_vdp2();

        vdp_sync_vblank_in_clear();
        vdp_sync_vblank_out_clear();

        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_IN, _vblank_in_handler);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_OUT, _vblank_out_handler);

        scu_ic_mask_chg(SCU_MASK_DISABLE, IC_MASK_NONE);

        vdp_sync_user_callback_clear();
}

void
vdp_sync(int16_t interval __unused)
{
        vdp1_sync_draw_wait();

        uint8_t sr_mask;
        sr_mask = cpu_intc_mask_get();

        uint32_t scu_mask;
        scu_mask = scu_ic_mask_get();

        /* Enter critical section */
        cpu_intc_mask_set(15);
        scu_ic_mask_chg(IC_MASK_ALL, SCU_MASK_ENABLE);

        assert((_state.sync & STATE_SYNC) == 0x00);

        _state.sync |= STATE_SYNC;

        scu_ic_mask_chg(SCU_MASK_DISABLE, IC_MASK_NONE);
        cpu_intc_mask_set(0);

        /* Wait until VDP1 changed frame buffers and wait until VDP2 state has
         * been committed. */
        bool vdp1_working;
        bool vdp2_working;
        do {
                vdp1_working = (_state.vdp1 & STATE_VDP1_REQUEST_CHANGE) != 0x00;

                bool vdp2_request_commit;
                vdp2_request_commit = (_state.vdp2 & STATE_VDP2_REQUEST_COMMIT) != 0x00;

                bool vdp2_committed;
                vdp2_committed = (_state.vdp2 & STATE_VDP2_COMMITTED) != 0x00;

                vdp2_working = vdp2_request_commit && !vdp2_committed;
        } while (vdp1_working || vdp2_working);

        cpu_intc_mask_set(15);
        scu_ic_mask_chg(IC_MASK_ALL, SCU_MASK_ENABLE);
        _state.sync &= ~STATE_SYNC;
        scu_ic_mask_chg(SCU_MASK_DISABLE, IC_MASK_NONE);
        cpu_intc_mask_set(0);

        uint32_t i;
        for (i = 0; i < USER_CALLBACK_COUNT; i++) {
                _user_callbacks[i](_user_works[i]);

                /* Remove callback as soon as it's done */
                _user_callbacks[i] = _default_user_callback;
                _user_works[i] = NULL;
        }

        scu_ic_mask_set(scu_mask);
        cpu_intc_mask_set(sr_mask);
}

void
vdp1_sync_draw(const struct vdp1_cmdt_list *cmdt_list)
{
        assert(cmdt_list != NULL);
        assert(cmdt_list->cmdts != NULL);
        assert(cmdt_list->cmdt != NULL);
        assert(cmdt_list->count > 0);

        uint16_t count;
        count = cmdt_list->cmdt - cmdt_list->cmdts;

        assert(count > 0);

        /* Wait as previous draw calls haven't yet been committed, or at the
         * very least, have the command list transferred to VRAM */
        vdp1_sync_draw_wait();

        _state.vdp1 |= STATE_VDP1_REQUEST_COMMIT_LIST;
        _state.vdp1 &= ~STATE_VDP1_LIST_COMMITTED;

        uint32_t xfer_len;
        xfer_len = count * sizeof(struct vdp1_cmdt);
        uint32_t xfer_dst;
        xfer_dst = VDP1_VRAM(_vdp1_last_command * sizeof(struct vdp1_cmdt));
        uint32_t xfer_src;
        xfer_src = CPU_CACHE_THROUGH | (uint32_t)cmdt_list->cmdts;

        _vdp1_dma_cfg.dlc_mode = DMA_MODE_DIRECT;
        _vdp1_dma_cfg.dlc_xfer.direct.len = xfer_len;
        _vdp1_dma_cfg.dlc_xfer.direct.dst = xfer_dst;
        _vdp1_dma_cfg.dlc_xfer.direct.src = xfer_src;
        _vdp1_dma_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        _vdp1_dma_cfg.dlc_update = DMA_UPDATE_NONE;

        scu_dma_config_buffer(&_vdp1_dma_reg_buffer, &_vdp1_dma_cfg);

        int8_t ret;
        ret = dma_queue_enqueue(&_vdp1_dma_reg_buffer, DMA_QUEUE_TAG_IMMEDIATE, _vdp1_dma_handler, NULL);
        assert(ret == 0);

        ret = dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);
        assert(ret >= 0);
}

void
vdp1_sync_draw_wait(void)
{
        if ((_state.vdp1 & STATE_VDP1_REQUEST_COMMIT_LIST) == 0x00) {
                return;
        }

        uint8_t sr_mask;
        sr_mask = cpu_intc_mask_get();

        cpu_intc_mask_set(0);

        uint32_t scu_mask;
        scu_mask = scu_ic_mask_get();

        scu_ic_mask_chg(SCU_MASK_DISABLE, IC_MASK_NONE);

        if (_interlace_mode_double()) {
                /* Wait for transfer only as we can't wait until VDP1 processes
                 * the command list */
                while ((_state.vdp1 & STATE_VDP1_LIST_TRANSFERRED) == 0x00) {
                }
        } else {
                /* Wait until VDP1 has processed the command list */
                while ((_state.vdp1 & STATE_VDP1_LIST_COMMITTED) == 0x00) {
                }

                _state.vdp1 &= ~STATE_VDP1_REQUEST_COMMIT_LIST;
                _state.vdp1 &= ~STATE_VDP1_LIST_COMMITTED;
        }

        _state.vdp1 |= STATE_VDP1_REQUEST_CHANGE;

        scu_ic_mask_set(scu_mask);
        cpu_intc_mask_set(sr_mask);
}

void
vdp2_sync_commit(void)
{
        _state.vdp2 |= STATE_VDP2_REQUEST_COMMIT;
        _state.vdp2 &= ~STATE_VDP2_COMMITTED;
        _state.vdp2 &= ~STATE_VDP2_REQUEST_PENDING;

        struct dma_reg_buffer *reg_buffer;
        reg_buffer = &_state_vdp2()->commit.reg_buffer;

        int8_t ret;
        ret = dma_queue_enqueue(reg_buffer, DMA_QUEUE_TAG_VBLANK_IN, _vdp2_commit_handler, NULL);
        assert(ret == 0);
}

uint16_t
vdp1_sync_last_command_get(void)
{
        return _vdp1_last_command;
}

void
vdp_sync_vblank_in_set(void (*ihr)(void))
{
        _user_vblank_in_handler = (ihr != NULL) ? ihr : _default_handler;
}

void
vdp_sync_vblank_out_set(void (*ihr)(void))
{
        _user_vblank_out_handler = (ihr != NULL) ? ihr : _default_handler;
}

int8_t
vdp_sync_user_callback_add(void (*user_callback)(void *), void *work)
{
        assert(user_callback != NULL);

        uint32_t i;
        for (i = 0; i < USER_CALLBACK_COUNT; i++) {
                if (_user_callbacks[i] != _default_user_callback) {
                        continue;
                }

                _user_callbacks[i] = user_callback;
                _user_works[i] = work;

                return i;
        }

        assert(i != USER_CALLBACK_COUNT);

        return -1;
}

void
vdp_sync_user_callback_remove(int8_t id)
{
        assert(id >= 0);
        assert(id < USER_CALLBACK_COUNT);

        _user_callbacks[id] = _default_user_callback;
        _user_works[id] = NULL;

}

void
vdp_sync_user_callback_clear(void)
{
        uint32_t i;
        for (i = 0; i < USER_CALLBACK_COUNT; i++) {
                _user_callbacks[i] = _default_user_callback;
                _user_works[i] = NULL;
        }
}

static void
_init_vdp1(void)
{
        _state.vdp1 &= ~STATE_VDP1_REQUEST_COMMIT_LIST;
        _state.vdp1 &= ~STATE_VDP1_LIST_COMMITTED;
        _state.vdp1 &= ~STATE_VDP1_REQUEST_CHANGE;
        _state.vdp1 &= ~STATE_VDP1_LIST_TRANSFERRED;

        _vdp1_last_command = 0x0000;

        scu_ic_ihr_set(IC_INTERRUPT_SPRITE_END, _sprite_end_handler);
}

static void
_init_vdp2(void)
{
        _state.vdp2 &= ~STATE_VDP2_REQUEST_COMMIT;
        _state.vdp2 &= ~STATE_VDP2_COMMITTED;
        _state.vdp2 &= ~STATE_VDP2_REQUEST_PENDING;

        struct dma_xfer *xfer;

        /* Write VDP2(TVMD) first */
        xfer = &_state_vdp2()->commit.xfer_table[COMMIT_XFER_VDP2_REG_TVMD];
        xfer->len = 2;
        xfer->dst = VDP2(0);
        xfer->src = CPU_CACHE_THROUGH | (uint32_t)&_state_vdp2()->regs.tvmd;

        /* Skip the first 8 VDP2 registers */
        xfer = &_state_vdp2()->commit.xfer_table[COMMIT_XFER_VDP2_REGS];
        xfer->len = sizeof(_state_vdp2()->regs) - 16;
        xfer->dst = VDP2(16);
        xfer->src = CPU_CACHE_THROUGH | (uint32_t)&_state_vdp2()->regs.buffer[8];

        xfer = &_state_vdp2()->commit.xfer_table[COMMIT_XFER_BACK_SCREEN_BUFFER];
        xfer->len = 0;
        xfer->dst = 0x00000000;
        xfer->src = 0x00000000;

        struct dma_level_cfg dma_level_cfg = {
                .dlc_mode = DMA_MODE_INDIRECT,
                .dlc_xfer.indirect = &_state_vdp2()->commit.xfer_table[0],
                .dlc_stride = DMA_STRIDE_2_BYTES,
                .dlc_update = DMA_UPDATE_NONE
        };

        void *reg_buffer;
        reg_buffer = &_state_vdp2()->commit.reg_buffer;

        scu_dma_config_buffer(reg_buffer, &dma_level_cfg);
}

static void
_vblank_in_handler(void)
{
        const uint16_t fbcr_bits[] = {
                /* Render even-numbered lines */
                0x0008,
                /* Render odd-numbered lines */
                0x000C
        };

        /* VBLANK-IN interrupt runs at scanline #224 */

        _state.sync |= STATE_SYNC_VBLANK_IN;
        _state.sync &= ~STATE_SYNC_VBLANK_OUT;

        if ((_state.sync & STATE_SYNC) == 0x00) {
                goto no_sync;
        }

        /* Commit VDP2 by default, unless double-density interlace mode is set
         * as we need to wait two field scans */
        bool commit_vdp2;
        commit_vdp2 = true;

        if ((_interlace_mode_double()) && ((_state.vdp1 & STATE_VDP1_LIST_TRANSFERRED) != 0x00)) {
                /* Assert for now, until we can perform a pseudo draw
                 * continuation */
                if ((_vdp1_transfer_over())) {
                        assert(false);

                        MEMORY_WRITE(16, VDP1(PTMR), 0x0000);

                        return;
                }

                /* Get even/odd field scan */
                uint8_t field_scan;
                field_scan = vdp2_tvmd_field_scan_get();

                if (field_scan == _state.field_count) {
                        MEMORY_WRITE(16, VDP1(FBCR), fbcr_bits[field_scan]);
                        MEMORY_WRITE(16, VDP1(PTMR), 0x0002);

                        _state.field_count++;
                }

                /* Don't commit VDP2 until we've completed two field scans */
                commit_vdp2 = (_state.field_count < 2);
        }

        bool vdp2_request_commit;
        vdp2_request_commit = (_state.vdp2 & STATE_VDP2_REQUEST_COMMIT) != 0x00;

        bool vdp2_request_pending;
        vdp2_request_pending = (_state.vdp2 & STATE_VDP2_REQUEST_PENDING) != 0x00;

        if (commit_vdp2 && vdp2_request_commit && !vdp2_request_pending) {
                _state.vdp2 |= STATE_VDP2_REQUEST_PENDING;

                int8_t ret;
                ret = dma_queue_flush(DMA_QUEUE_TAG_VBLANK_IN);
                assert(ret >= 0);
        }

no_sync:
        _user_vblank_in_handler();
}

static void
_vblank_out_handler(void)
{
        /* VBLANK-OUT interrupt runs at scanline #511 */

        _state.sync |= STATE_SYNC_VBLANK_OUT;
        _state.sync &= ~STATE_SYNC_VBLANK_IN;

        if ((_state.sync & STATE_SYNC) == 0x00) {
                goto no_sync;
        }

        if ((_state.vdp1 & STATE_VDP1_REQUEST_CHANGE) == 0x00) {
                goto no_sync;
        }

        if ((_interlace_mode_double())) {
                /* If we've completed two field scans */
                if (_state.field_count == 2) {
                        _state.field_count = 0;

                        _state.vdp1 &= ~STATE_VDP1_REQUEST_COMMIT_LIST;
                        _state.vdp1 &= ~STATE_VDP1_LIST_COMMITTED;
                        _state.vdp1 &= ~STATE_VDP1_REQUEST_CHANGE;
                        _state.vdp1 &= ~STATE_VDP1_LIST_TRANSFERRED;

                        /* Reset command address to the top */
                        _vdp1_last_command = 0x0000;
                }
        } else {
                /* Manual mode (change) */
                MEMORY_WRITE(16, VDP1(FBCR), 0x0003);

                _state.vdp1 &= ~STATE_VDP1_REQUEST_CHANGE;
                _state.vdp1 &= ~STATE_VDP1_LIST_TRANSFERRED;

                /* Reset command address to the top */
                _vdp1_last_command = 0x0000;
        }

no_sync:
        _user_vblank_out_handler();
}

static void
_sprite_end_handler(void)
{
        _vdp1_last_command = vdp1_cmdt_current_get();

        /* VDP1 request to commit is finished */
        _state.vdp1 |= STATE_VDP1_LIST_COMMITTED;
}

static void
_vdp1_dma_handler(const struct dma_queue_transfer *transfer __unused)
{
        _state.vdp1 |= STATE_VDP1_LIST_TRANSFERRED;

        if ((_interlace_mode_double())) {
                return;
        }

        /* Since the DMA transfer went through, the VDP1 is idling, so start
         * drawing */
        MEMORY_WRITE(16, VDP1(PTMR), 0x0001);
}

static void
_vdp2_commit_handler(const struct dma_queue_transfer *transfer __unused)
{
        /* VDP2 request to commit is finished */
        _state.vdp2 &= ~STATE_VDP2_REQUEST_COMMIT;
        _state.vdp2 |= STATE_VDP2_COMMITTED;
        _state.vdp2 &= ~STATE_VDP2_REQUEST_PENDING;
}

static void
_default_handler(void)
{
}

static inline bool __always_inline
_interlace_mode_double(void)
{
        return (_state_vdp2()->tv.interlace == TVMD_INTERLACE_DOUBLE);
}

static inline bool __always_inline
_vdp1_transfer_over(void)
{
        struct vdp1_transfer_status transfer_status;
        vdp1_transfer_status_get(&transfer_status);

        struct vdp1_mode_status mode_status;
        vdp1_mode_status_get(&mode_status);

        /* Detect if VDP1 is still drawing (transfer over status) */
        bool transfer_over;
        transfer_over = (transfer_status.vte_bef == 0x00) &&
                        (transfer_status.vte_cef == 0x00);

        return ((mode_status.vms_ptm1 != 0x00) && transfer_over);
}

static void
_default_user_callback(void *work __unused)
{
}
