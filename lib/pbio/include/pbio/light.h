// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup Light Light control functions
 * @{
 */

#ifndef _PBIO_LIGHT_H_
#define _PBIO_LIGHT_H_

#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>

/** Color light instance. */
typedef struct _pbio_color_light_t pbio_color_light_t;

/** Single element of a color light animation. */
typedef struct {
    /** The color and brightness for this cell. See pbio_color_light_on_hsv(). */
    pbio_color_hsv_t hsv;
    /** The duration of this cell in milliseconds. */
    uint32_t duration;
} pbio_color_light_animation_cell_t;

/**
 * Convience macro for defining ::pbio_color_light_animation_cell_t cells.
 * @param [in]  h_  The hue (0 to 360)
 * @param [in]  s_  The saturation (0 to 100)
 * @param [in]  v_  The brightness (0 to 100)
 * @param [in]  d   The duration in milliseconds (> 0)
 */
#define PBIO_COLOR_LIGHT_ANIMATION_CELL(h_, s_, v_, d) \
    { .hsv = { .h = (h_), .s = (s_), .v = (v_) }, .duration = (d) }

/** Sentinel value for a color light animation array. */
#define PBIO_COLOR_LIGHT_ANIMATION_END { .duration = 0 }

#if PBIO_CONFIG_LIGHT

pbio_error_t pbio_color_light_on_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv);
pbio_error_t pbio_color_light_on(pbio_color_light_t *light, pbio_color_t color);
pbio_error_t pbio_color_light_off(pbio_color_light_t *light);
void pbio_color_light_start_animation(pbio_color_light_t *light, const pbio_color_light_animation_cell_t *cells);
void pbio_color_light_stop_animation(pbio_color_light_t *light);

#else // PBIO_CONFIG_LIGHT

static inline pbio_error_t pbio_color_light_on_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_color_light_on(pbio_color_light_t *light, pbio_color_t color) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_color_light_off(pbio_color_light_t *light) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_color_light_start_animation(pbio_color_light_t *light, const pbio_color_light_animation_cell_t *cells) {
}

static inline void pbio_color_light_stop_animation(pbio_color_light_t *light) {
}

#endif // PBIO_CONFIG_LIGHT

#endif // _PBIO_LIGHT_H_

/** @}*/
