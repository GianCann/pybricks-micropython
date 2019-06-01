// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "py/mpconfig.h"

#if PYBRICKS_PY_ADVANCED || PYBRICKS_PY_PUPDEVICES

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"

#include "pberror.h"
#include "pbiodevice.h"

void pb_iodevice_assert_type_id(pbio_iodev_t *iodev, pbio_iodev_type_id_t type_id) {
    if (!iodev->info || iodev->info->type_id != type_id) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }
}

pbio_error_t pb_iodevice_get_type_id(pbio_iodev_t *iodev, pbio_iodev_type_id_t *id) {
    if (!iodev->info) {
        return PBIO_ERROR_NO_DEV;
    }
    *id = iodev->info->type_id;
    return PBIO_SUCCESS;
}

pbio_error_t pb_iodevice_get_mode(pbio_iodev_t *iodev, uint8_t *current_mode) {
    *current_mode = iodev->mode;
    return PBIO_SUCCESS;
}

pbio_error_t pb_iodevice_set_mode(pbio_iodev_t *iodev, uint8_t new_mode) {
    pbio_error_t err;

    // FIXME: it would be better to do this check on a per-sensor basis since
    // some sensors use setting the mode as a oneshot to update the sensor
    // value - e.g. LEGO EV3 Ultrasonic sensor in certain modes.
    if (iodev->mode == new_mode){
        return PBIO_SUCCESS;
    }

    err = pbio_iodev_set_mode(iodev, new_mode);
    // Wait for mode change to complete unless there was an error.
    while (err == PBIO_SUCCESS && iodev->mode != new_mode) {
        mp_hal_delay_ms(1);
    }
    return err;
}

mp_obj_t pb_iodevice_get_values(pbio_iodev_t *iodev) {
    mp_obj_t values[PBIO_IODEV_MAX_DATA_SIZE];
    uint8_t *data;
    uint8_t len, i;
    pbio_iodev_data_type_t type;

    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    pb_assert(pbio_iodev_get_bin_format(iodev, &len, &type));

    // this shouldn't happen, but just in case...
    if (len == 0) {
        return mp_const_none;
    }

    for (i = 0; i < len; i++) {
        switch (type) {
        case PBIO_IODEV_DATA_TYPE_INT8:
            values[i] = mp_obj_new_int(data[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_INT16:
            values[i] = mp_obj_new_int(*(int16_t *)(data + i * 2));
            break;
        case PBIO_IODEV_DATA_TYPE_INT32:
            values[i] = mp_obj_new_int(*(int32_t *)(data + i * 4));
            break;
        case PBIO_IODEV_DATA_TYPE_FLOAT:
            #if MICROPY_PY_BUILTINS_FLOAT
            values[i] = mp_obj_new_float(*(float *)(data + i * 4));
            #else // MICROPY_PY_BUILTINS_FLOAT
            // there aren't any known devices that use float data, so hopefully we will never hit this
            mp_raise_OSError(MP_EOPNOTSUPP);
            #endif // MICROPY_PY_BUILTINS_FLOAT
            break;
        default:
            mp_raise_NotImplementedError("Unknown data type");
        }
    }

    // if there are more than one value, pack them in a tuple
    if (len > 1) {
        return mp_obj_new_tuple(len, values);
    }

    // otherwise return the one value
    return values[0];
}

mp_obj_t pb_iodevice_set_values(pbio_iodev_t *iodev, mp_obj_t values) {
    uint8_t data[PBIO_IODEV_MAX_DATA_SIZE];
    mp_obj_t *items;
    uint8_t len, i;
    pbio_iodev_data_type_t type;

    pb_assert(pbio_iodev_get_bin_format(iodev, &len, &type));

    // if we only have one value, it doesn't have to be a tuple/list
    if (len == 1 && (mp_obj_is_integer(values)
        #if MICROPY_PY_BUILTINS_FLOAT
        || mp_obj_is_float(values)
        #endif
    )) {
        items = &values;
    }
    else {
        mp_obj_get_array_fixed_n(values, len, &items);
    }

    for (i = 0; i < len; i++) {
        switch (type) {
        case PBIO_IODEV_DATA_TYPE_INT8:
            data[i] = mp_obj_get_int(items[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_INT16:
            *(int16_t *)(data + i * 2) = mp_obj_get_int(items[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_INT32:
            *(int32_t *)(data + i * 4) = mp_obj_get_int(items[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_FLOAT:
            #if MICROPY_PY_BUILTINS_FLOAT
            *(float *)(data + i * 4) = mp_obj_get_float(items[i]);
            #else // MICROPY_PY_BUILTINS_FLOAT
            // there aren't any known devices that use float data, so hopefully we will never hit this
            mp_raise_OSError(MP_EOPNOTSUPP);
            #endif // MICROPY_PY_BUILTINS_FLOAT
            break;
        default:
            mp_raise_NotImplementedError("Unknown data type");
        }
    }

    pb_assert(pbio_iodev_set_raw_values(iodev, data));
    return mp_const_none;
}

#endif // PYBRICKS_PY_ADVANCED || PYBRICKS_PY_PUPDEVICES