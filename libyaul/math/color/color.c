/*
 * Copyright (c) 2013-2014
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <string.h>

#include "color.h"

void
rgb1555_rgb888_conv(const rgb1555_t *rgb1555, rgb888_t *result)
{
        *result = RGB1555_RGB888(color->msb, rgb1555->r, rgb1555->g, rgb1555->b);
}

void
rgb888_rgb1555_conv(const rgb888_t *rgb888, rgb1555_t *result)
{
        *result = RGB888_RGB1555(0, rgb888->r, rgb888->g, rgb888->b);
}

void
rgb888_hsv_conv(const rgb888_t *rgb888 __unused, hsv_t *result __unused)
{
}
