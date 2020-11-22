/*
 * Copyright (c) 2020
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <sys/cdefs.h>

#include <vdp.h>
#include <vdp2/tvmd.h>

#include <cpu/cache.h>
#include <cpu/divu.h>
#include <cpu/frt.h>

#include "sega3d.h"
#include "sega3d-internal.h"

extern void _internal_fog_init(void);
extern void _internal_matrix_init(void);
extern void _internal_sort_init(void);
extern void _internal_tlist_init(void);
extern void _internal_transform_init(void);

static void _perspective_calculate(FIXED fov_angle);
static void _frustrum_nf_clip_planes_calculate(void);
static void _frustum_clip_planes_calculate(FIXED fov_angle);

void
sega3d_init(void)
{        
        /* Prevent re-initialization */
        if ((_internal_state->flags & FLAGS_INITIALIZED) != FLAGS_NONE) {
                return;
        }

        _internal_state->flags = FLAGS_INITIALIZED;

        (void)memset(_internal_state->info, 0, sizeof(sega3d_info_t));

        sega3d_display_level_set(0);
        sega3d_perspective_set(DEGtoANG(90.0f));

        _internal_tlist_init();
        _internal_matrix_init();
        _internal_fog_init();
        _internal_sort_init();
        _internal_transform_init();
}

void
sega3d_display_level_set(uint16_t level)
{
        sega3d_info_t * const info = _internal_state->info;

        info->level = level & (DISPLAY_LEVEL_COUNT - 1);
        info->near = info->view_distance >> info->level;
        info->far = FAR_Z;

        _frustrum_nf_clip_planes_calculate();
}

void
sega3d_perspective_set(ANGLE fov)
{
        if (fov < MIN_FOV_ANGLE) {
                fov = MIN_FOV_ANGLE;
        }

        if (fov > MAX_FOV_ANGLE) {
                fov = MAX_FOV_ANGLE;
        }
        sega3d_info_t * const info = _internal_state->info;

        const FIXED fov_angle = fix16_mul(fov, FIX16_2PI) >> 1;

        info->fov = fov;

        _perspective_calculate(fov_angle);
        _frustum_clip_planes_calculate(fov_angle);
}

void
sega3d_frustum_camera_set(const POINT position, const VECTOR rx,
    const VECTOR ry, const VECTOR rz)
{
        FIXED * const clip_camera = (FIXED *)_internal_state->clip_camera;

        clip_camera[M00] = rx[X];
        clip_camera[M01] = rx[Y];
        clip_camera[M02] = rx[Z];
        clip_camera[M03] = position[X];

        clip_camera[M10] = ry[X];
        clip_camera[M11] = ry[Y];
        clip_camera[M12] = ry[Z];
        clip_camera[M13] = position[Y];

        clip_camera[M20] = rz[X];
        clip_camera[M21] = rz[Y];
        clip_camera[M22] = rz[Z];
        clip_camera[M23] = position[Z];
}

void
sega3d_info_get(sega3d_info_t *info)
{
        (void)memcpy(info, _internal_state->info, sizeof(sega3d_info_t));
}

Uint16
sega3d_object_polycount_get(const sega3d_object_t *object)
{
        const PDATA * const pdata = object->pdatas;

        return pdata->nbPolygon;
}

static void
_perspective_calculate(FIXED fov_angle)
{
        transform_t * const trans = _internal_state->transform;
        sega3d_info_t * const info = _internal_state->info;

        uint16_t i_width;
        uint16_t i_height;

        vdp2_tvmd_display_res_get(&i_width, &i_height);

        /* Let's maintain a 4:3 ratio */
        if (i_height < 240) {
                i_height = 240;
        }

        const FIXED screen_width = fix16_int32_from(i_width);
        const FIXED screen_height = fix16_int32_from(i_height);

        trans->cached_sw_2 = i_width / 2;
        trans->cached_sh_2 = i_height / 2;

        cpu_divu_fix16_set(screen_width, screen_height);
        info->ratio = cpu_divu_quotient_get();

        info->view_distance = fix16_mul(screen_width - FIX16(1.0f), fix16_tan(fov_angle)) >> 1;
        info->near = info->view_distance >> info->level;
}

static void
_frustrum_nf_clip_planes_calculate(void)
{
        const sega3d_info_t * const info = _internal_state->info;
        clip_planes_t * const clip_planes = _internal_state->clip_planes;

        fix16_vec3_t * const near_d = &clip_planes->near_d;
        near_d->x = FIX16(0.0f);
        near_d->y = FIX16(0.0f);
        near_d->z = info->near;

        fix16_vec3_t * const far_d = &clip_planes->far_d;
        far_d->x = FIX16(0.0f);
        far_d->y = FIX16(0.0f);
        far_d->z = info->far;
}

static void
_frustum_clip_planes_calculate(FIXED fov_angle)
{
        static const fix16_vec3_t axis_up    = FIX16_VEC3_INITIALIZER(0.0f, -1.0f, 0.0f);
        static const fix16_vec3_t axis_right = FIX16_VEC3_INITIALIZER(1.0f,  0.0f, 0.0f);

        const sega3d_info_t * const info = _internal_state->info;
        clip_planes_t * const clip_planes = _internal_state->clip_planes;

        const FIXED aspect_ratio = info->ratio;

        const FIXED sin = fix16_sin(fov_angle);
        const FIXED cos = fix16_sin(fov_angle);
        cpu_divu_fix16_set(FIX16(1.0f), cos);
        const FIXED inv_cos = cpu_divu_quotient_get();

        fix16_vec3_t right;
        right.x = fix16_mul(inv_cos, sin);
        right.y = FIX16(0.0f);
        right.z = FIX16(1.0f);

        fix16_vec3_t top;
        top.x = FIX16(0.0f);
        top.y = fix16_mul(aspect_ratio, right.x);
        top.z = aspect_ratio;

        fix16_vec3_t * const near_normal = &clip_planes->near_normal;
        near_normal->x = FIX16(0.0f);
        near_normal->y = FIX16(0.0f);
        near_normal->z = FIX16(1.0f);

        fix16_vec3_t * const far_normal = &clip_planes->far_normal;
        far_normal->x = FIX16( 0.0f);
        far_normal->y = FIX16( 0.0f);
        far_normal->z = FIX16(-1.0f);

        fix16_vec3_t * const right_normal = &clip_planes->right_normal;
        fix16_vec3_cross(&axis_up, &right, right_normal);
        fix16_vec3_normalize(right_normal);

        fix16_vec3_t * const left_normal = &clip_planes->left_normal;
        left_normal->x = -right_normal->x;
        left_normal->y =  right_normal->y;
        left_normal->z =  right_normal->z;

        fix16_vec3_t * const top_normal = &clip_planes->top_normal;
        fix16_vec3_cross(&axis_right, &top, top_normal);
        fix16_vec3_normalize(top_normal);

        _frustrum_nf_clip_planes_calculate();
}
