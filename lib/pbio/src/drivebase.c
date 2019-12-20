// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk


#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_drivebase_t base;

static pbio_error_t error_or(pbio_error_t left, pbio_error_t right) {
    if (left != PBIO_SUCCESS) {
        return left;
    }
    return right;
}

static pbio_error_t pbio_drivebase_setup(pbio_drivebase_t *drivebase,
                                         pbio_servo_t *left,
                                         pbio_servo_t *right,
                                         fix16_t wheel_diameter,
                                         fix16_t axle_track) {
    pbio_error_t err;

    // Reset both motors to a passive state
    err = pbio_servo_stop(left, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_stop(right, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Individual servos
    drivebase->left = left;
    drivebase->right = right;

    // Drivebase geometry
    if (wheel_diameter <= 0 || axle_track <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    drivebase->wheel_diameter = wheel_diameter;
    drivebase->axle_track = axle_track;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_get(pbio_drivebase_t **drivebase, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track) {
    // Unique pointer to drivebase device
    *drivebase = &base;

    // Configure drivebase and set properties
    return pbio_drivebase_setup(*drivebase, left, right, wheel_diameter, axle_track);
}

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *drivebase, pbio_actuation_t after_stop) {

    pbio_error_t err_l, err_r;

    switch (after_stop) {
        case PBIO_ACTUATION_COAST:
            // Stop by coasting
            err_l = pbio_hbridge_coast(drivebase->left->hbridge);
            err_r = pbio_hbridge_coast(drivebase->right->hbridge);
            return error_or(err_l, err_r);
        case PBIO_ACTUATION_BRAKE:
            // Stop by braking
            err_l = pbio_hbridge_brake(drivebase->left->hbridge);
            err_r = pbio_hbridge_brake(drivebase->right->hbridge);
            return error_or(err_l, err_r);
        default:
            // HOLD is not implemented
            return PBIO_ERROR_INVALID_ARG;
    }
}

pbio_error_t pbio_drivebase_start(pbio_drivebase_t *drivebase, int32_t speed, int32_t rate) {

    pbio_error_t err_l, err_r;

    // FIXME: This is a fake drivebase without synchronization
    int32_t sum = 180 * pbio_math_mul_i32_fix16(pbio_math_div_i32_fix16(speed, drivebase->wheel_diameter), FOUR_DIV_PI);
    int32_t dif = 2 * pbio_math_div_i32_fix16(pbio_math_mul_i32_fix16(rate, drivebase->axle_track), drivebase->wheel_diameter);

    err_l = pbio_servo_run(drivebase->left, (sum+dif)/2);
    err_r = pbio_servo_run(drivebase->right, (sum-dif)/2);

    return error_or(err_l, err_r);
}

static pbio_error_t pbio_drivebase_update(pbio_drivebase_t *drivebase) {
    return PBIO_SUCCESS;
}

// TODO: Convert to Contiki process

// Service all drivebase motors by calling this function at approximately constant intervals.
void _pbio_drivebase_poll(void) {
    pbio_drivebase_t *drivebase = &base;

    if (drivebase->left && drivebase->left->connected && drivebase->right && drivebase->right->connected) {
        pbio_drivebase_update(drivebase);
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
