/*
 * Copyright (c) 2012-2022
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 * Romulo Fernandes <abra185@gmail.com>
 */

#ifndef _YAUL_MATH_FIX16_H_
#error "Header file must not be directly included"
#endif /* !_YAUL_MATH_FIX16_H_ */

#define FIX16_VEC3_INITIALIZER(x, y, z)                                        \
    {                                                                          \
            {                                                                  \
                    FIX16(x),                                                  \
                    FIX16(y),                                                  \
                    FIX16(z)                                                   \
            }                                                                  \
    }

typedef union fix16_vec3 {
        struct {
                fix16_t x;
                fix16_t y;
                fix16_t z;
        };

        fix16_t comp[3];
} __packed __aligned(4) fix16_vec3_t;

static inline void __always_inline
fix16_vec3_zero(fix16_vec3_t *result)
{
        result->x = FIX16(0.0f);
        result->y = FIX16(0.0f);
        result->z = FIX16(0.0f);
}

static inline void __always_inline
fix16_vec3_set(fix16_vec3_t *result, fix16_t x, fix16_t y, fix16_t z)
{
        result->x = x;
        result->y = y;
        result->z = z;
}

static inline void __always_inline
fix16_vec3_dup(const fix16_vec3_t * __restrict v0,
    fix16_vec3_t * __restrict result)
{
        result->x = v0->x;
        result->y = v0->y;
        result->z = v0->z;
}

static inline void __always_inline
fix16_vec3_add(const fix16_vec3_t * __restrict v0,
    const fix16_vec3_t * __restrict v1, fix16_vec3_t * __restrict result)
{
        result->x = v0->x + v1->x;
        result->y = v0->y + v1->y;
        result->z = v0->z + v1->z;
}

static inline void __always_inline
fix16_vec3_sub(const fix16_vec3_t * __restrict v1,
    const fix16_vec3_t * __restrict const v0, fix16_vec3_t * __restrict const result)
{
        result->x = v1->x - v0->x;
        result->y = v1->y - v0->y;
        result->z = v1->z - v0->z;
}

static inline void __always_inline
fix16_vec3_scale(const fix16_t scalar, fix16_vec3_t *result)
{
        result->x = fix16_mul(scalar, result->x);
        result->y = fix16_mul(scalar, result->y);
        result->z = fix16_mul(scalar, result->z);
}

static inline void __always_inline
fix16_vec3_scaled(const fix16_t scalar, const fix16_vec3_t * __restrict v,
    fix16_vec3_t * __restrict result)
{
        result->x = fix16_mul(scalar, v->x);
        result->y = fix16_mul(scalar, v->y);
        result->z = fix16_mul(scalar, v->z);
}

static inline fix16_t __always_inline
fix16_vec3_inline_dot(const fix16_vec3_t *a, const fix16_vec3_t *b)
{
        __register uint32_t aux0;
        __register uint32_t aux1;

        __asm__ volatile ("\tclrmac\n"
                          "\tmac.l @%[a]+, @%[b]+\n"
                          "\tmac.l @%[a]+, @%[b]+\n"
                          "\tmac.l @%[a]+, @%[b]+\n"
                          "\tsts mach, %[aux0]\n"
                          "\tsts macl, %[aux1]\n"
                          "\txtrct %[aux0], %[aux1]\n"
            : [a] "+r" (a),
              [b] "+r" (b),
              [aux0] "=&r" (aux0),
              [aux1] "=&r" (aux1)
            : "m" (*a),
              "m" (*b)
            : "mach", "macl", "memory");

        return aux1;
}

extern fix16_t fix16_vec3_length(const fix16_vec3_t *v0);
extern fix16_t fix16_vec3_sqr_length(const fix16_vec3_t *v0);
extern void fix16_vec3_normalize(fix16_vec3_t *v0);
extern void fix16_vec3_normalized(const fix16_vec3_t * __restrict v0,
    fix16_vec3_t * __restrict result);
extern fix16_t fix16_vec3_dot(const fix16_vec3_t *v0, const fix16_vec3_t *v1);
extern void fix16_vec3_cross(const fix16_vec3_t * const __restrict v0,
    const fix16_vec3_t * const __restrict v1, fix16_vec3_t * __restrict result);

extern fix16_t fix16_vec3_cross_mag(const fix16_vec3_t * __restrict v0,
    const fix16_vec3_t * __restrict v1);

extern uint32_t fix16_vec3_str(const fix16_vec3_t *v0, char *buffer, int decimals);
