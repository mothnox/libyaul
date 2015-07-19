/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>
#include <fixmath.h>
#include <tga.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../test.h"
#include "../common.h"
#include "../fs.h"

static struct vdp1_cmdt_sprite sprite[6];

/* File handles */
static void *file_handle[32] __unused;

static uint32_t cram[6] __unused = {
        CRAM_VDP1_OFFSET(0,  0, 0), /* 16 colors, color mode 0 */
        CLUT(1, 0), /* 16 colors, color mode 1 */
        CRAM_VDP1_OFFSET(0,  4, 0), /* 64 colors, color mode 2 */
        CRAM_VDP1_OFFSET(0,  8, 0), /* 128 colors, color mode 3 */
        CRAM_VDP1_OFFSET(1,  0, 0), /* 256 colors, color mode 4 */
        0 /* RGB (32,768 colors), color mode 5 */
};

static uint16_t color_banks[6] __unused = {
        0, /* Color mode 0 */
        0, /* Color mode 1 */
        1, /* Color mode 2 */
        1, /* Color mode 3 */
        1, /* Color mode 4 */
        0 /* Color mode 5 */
};

void
test_02_init(void)
{
        test_init();

        uint32_t vram;
        vram = CHAR(0);

        size_t file_size;

        file_handle[0] = fs_open("/TESTS/02/COLOR_MODE0.TGA");
        file_handle[1] = fs_open("/TESTS/02/COLOR_MODE1.TGA");
        file_handle[2] = fs_open("/TESTS/02/COLOR_MODE2.TGA");
        file_handle[3] = fs_open("/TESTS/02/COLOR_MODE3.TGA");
        file_handle[4] = fs_open("/TESTS/02/COLOR_MODE4.TGA");
        file_handle[5] = fs_open("/TESTS/02/COLOR_MODE5.TGA");

        uint32_t mode_idx;
        for (mode_idx = 0; mode_idx <= 5; mode_idx++) {
                void *fh;
                fh = file_handle[mode_idx];
                file_size = common_round_pow2(fs_size(fh));

                uint8_t *ptr;
                ptr = (uint8_t *)0x00201000;

                fs_read(fh, ptr);
                fs_close(fh);

                tga_t tga;
                int status;
                status = tga_read(&tga, ptr);
                assert(status == TGA_FILE_OK);
                uint32_t amount;
                amount = tga_image_decode(&tga, (void *)vram);
                assert(amount > 0);
                if ((tga.tga_type == TGA_IMAGE_TYPE_CMAP) ||
                    (tga.tga_type == TGA_IMAGE_TYPE_RLE_CMAP)) {
                        amount = tga_cmap_decode(&tga, (uint16_t *)cram[mode_idx]);
                        assert(amount > 0);
                }

                sprite[mode_idx].cs_char = vram;

                vram += file_size;
        }
}

void
test_02_update(void)
{
        if (digital_pad.connected == 1) {
                if (digital_pad.held.button.start) {
                        test_exit();
                }
        }

        sprite[0].cs_type = CMDT_TYPE_NORMAL_SPRITE;
        sprite[0].cs_mode.cc_mode = 0;
        sprite[0].cs_mode.color_mode = 0;
        sprite[0].cs_color_bank = color_banks[0];
        sprite[0].cs_mode.transparent_pixel = true;
        sprite[0].cs_position.x = -128;
        sprite[0].cs_position.y = -80;
        sprite[0].cs_width = 64;
        sprite[0].cs_height = 64;

        sprite[1].cs_type = CMDT_TYPE_NORMAL_SPRITE;
        sprite[1].cs_mode.cc_mode = 0;
        sprite[1].cs_mode.color_mode = 1;
        sprite[1].cs_mode.transparent_pixel = true;
        sprite[1].cs_clut = CLUT(1, 0);
        sprite[1].cs_position.x = -32;
        sprite[1].cs_position.y = -80;
        sprite[1].cs_width = 64;
        sprite[1].cs_height = 64;

        sprite[2].cs_type = CMDT_TYPE_NORMAL_SPRITE;
        sprite[2].cs_mode.cc_mode = 0;
        sprite[2].cs_mode.color_mode = 2;
        sprite[2].cs_mode.transparent_pixel = true;
        sprite[2].cs_color_bank = color_banks[2];
        sprite[2].cs_position.x = 64;
        sprite[2].cs_position.y = -80;
        sprite[2].cs_width = 64;
        sprite[2].cs_height = 64;

        sprite[3].cs_type = CMDT_TYPE_NORMAL_SPRITE;
        sprite[3].cs_mode.cc_mode = 0;
        sprite[3].cs_mode.color_mode = 3;
        sprite[3].cs_mode.transparent_pixel = true;
        sprite[3].cs_color_bank = color_banks[3];
        sprite[3].cs_position.x = -128;
        sprite[3].cs_position.y = 16;
        sprite[3].cs_width = 64;
        sprite[3].cs_height = 64;

        sprite[4].cs_type = CMDT_TYPE_NORMAL_SPRITE;
        sprite[4].cs_mode.cc_mode = 0;
        sprite[4].cs_mode.color_mode = 4;
        sprite[4].cs_mode.transparent_pixel = true;
        sprite[4].cs_color_bank = color_banks[4];
        sprite[4].cs_position.x = -32;
        sprite[4].cs_position.y = 16;
        sprite[4].cs_width = 64;
        sprite[4].cs_height = 64;

        sprite[5].cs_type = CMDT_TYPE_NORMAL_SPRITE;
        sprite[5].cs_mode.cc_mode = 0;
        sprite[5].cs_mode.color_mode = 5;
        sprite[5].cs_mode.transparent_pixel = true;
        sprite[5].cs_position.x = 64;
        sprite[5].cs_position.y = 16;
        sprite[5].cs_width = 64;
        sprite[5].cs_height = 64;

        vdp1_cmdt_list_begin(0); {
                vdp1_cmdt_local_coord_set(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
                vdp1_cmdt_sprite_draw(&sprite[0]);
                vdp1_cmdt_sprite_draw(&sprite[1]);
                vdp1_cmdt_sprite_draw(&sprite[2]);
                vdp1_cmdt_sprite_draw(&sprite[3]);
                vdp1_cmdt_sprite_draw(&sprite[4]);
                vdp1_cmdt_sprite_draw(&sprite[5]);
                vdp1_cmdt_end();
        } vdp1_cmdt_list_end(0);
}

void
test_02_draw(void)
{
        vdp1_cmdt_list_commit();
}

void
test_02_exit(void)
{
}
