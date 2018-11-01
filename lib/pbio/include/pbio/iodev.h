
#ifndef _PBIO_IODEV_H_
#define _PBIO_IODEV_H_

#include <stdint.h>

#include "pbio/port.h"

/**
 * Type identifiers for I/O devices.
 *
 * UART device type IDs are hard-coded in the device, so those numbers can't
 * change. Other ID numbers are arbitrary.
 */
typedef enum {
    PBIO_IODEV_TYPE_ID_NONE                     = 0,    /**< No device is present */

    // LEGO Powered Up non-UART devices - some of these don't exist in real-life
    PBIO_IODEV_TYPE_ID_LPF2_MMOTOR              = 1,    /**< 45303 Powered Up Medium Motor (aka WeDo 2.0 motor) */
    PBIO_IODEV_TYPE_ID_LPF2_TRAIN               = 2,    /**< Powered Up Train Motor */
    /** @cond INTERNAL */
    PBIO_IODEV_TYPE_ID_LPF2_TURN                = 3,
    PBIO_IODEV_TYPE_ID_LPF2_POWER               = 4,
    PBIO_IODEV_TYPE_ID_LPF2_TOUCH               = 5,
    PBIO_IODEV_TYPE_ID_LPF2_LMOTOR              = 6,
    PBIO_IODEV_TYPE_ID_LPF2_XMOTOR              = 7,
    /** @endcond */
    PBIO_IODEV_TYPE_ID_LPF2_LIGHT               = 8,    /**< 88005 Powered Up Lights */
    /** @cond INTERNAL */
    PBIO_IODEV_TYPE_ID_LPF2_LIGHT1              = 9,
    PBIO_IODEV_TYPE_ID_LPF2_LIGHT2              = 10,
    PBIO_IODEV_TYPE_ID_LPF2_TPOINT              = 11,
    PBIO_IODEV_TYPE_ID_LPF2_EXPLOD              = 12,
    PBIO_IODEV_TYPE_ID_LPF2_3_PART              = 13,
    /** @endcond */
    PBIO_IODEV_TYPE_ID_LPF2_UNKNOWN_UART        = 14,   /**< Temporary ID for UART devices until real ID is read from the device */

    // LEGO EV3 UART devices
    PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR         = 29,   /**< MINDSTORMS EV3 Color Sensor */
    PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR    = 30,   /**< MINDSTORMS EV3 Ultrasonic Sensor */
    PBIO_IODEV_TYPE_ID_EV3_GYRO_SENSOR          = 32,   /**< MINDSTORMS EV3 Gyro Sensor */
    PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR            = 33,   /**< MINDSTORMS EV3 Infrared Sensor */

    // WeDo 2.0 UART devices
    PBIO_IODEV_TYPE_ID_WEDO2_TILT_SENSOR        = 34,   /**< WeDo 2.0 Tilt Sensor */
    PBIO_IODEV_TYPE_ID_WEDO2_MOTION_SENSOR      = 35,   /**< WeDo 2.0 Motion Sensor */
    PBIO_IODEV_TYPE_ID_WEDO2_GENERIC_SENSOR     = 36,

    // BOOST UART devices
    PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR        = 37,   /**< BOOST Color and Distance Sensor */
    PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR        = 38,   /**< BOOST Interactive Motor */

    // FatcatLab EV3 UART devices
    PBIO_IODEV_TYPE_ID_FCL_ADC                  = 71,   /**< FatcatLab A/DC Adapter */
    PBIO_IODEV_TYPE_ID_FCL_GESTURE              = 72,   /**< FatcatLab Gesture Sensor */
    PBIO_IODEV_TYPE_ID_FCL_LIGHT                = 73,   /**< FatcatLab Light Sensor */
    PBIO_IODEV_TYPE_ID_FCL_ALTITUDE             = 74,   /**< FatcatLab Altitude Sensor */
    PBIO_IODEV_TYPE_ID_FCL_IR                   = 75,   /**< FatcatLab IR Receiver */
    PBIO_IODEV_TYPE_ID_FCL_9DOF                 = 76,   /**< FatcatLab 9DOF Sensor */
    PBIO_IODEV_TYPE_ID_FCL_HUMIDITY             = 77,   /**< FatcatLab Humidity Sensor */
} pbio_iodev_type_id_t;

/**
 * Data types used by I/O devices.
 */
typedef enum {
    /**
     * Signed 8-bit integer.
     */
    PBIO_DATA_TYPE_INT8,
    /**
     * Little-endian, signed 16-bit integer.
     */
    PBIO_DATA_TYPE_INT16,
    /**
     * Little-endian, signed 32-bit integer.
     */
    PBIO_DATA_TYPE_INT32,
    /**
     * Little endian 32-bit floating point.
     */
    PBIO_DATA_TYPE_FLOAT,
    /**
     * The number of enum values in ::pbio_iodev_data_type_t.
     */
    PBIO_NUM_DATA_TYPE
} pbio_iodev_data_type_t;

/**
 * Max size of mode name (not including null terminator)
 */
#define PBIO_IODEV_MODE_NAME_SIZE   11

/**
 * Max size of units of measurements.
 */
#define PBIO_IODEV_UOM_SIZE         4

/**
 * Information about one mode of an I/O device.
 */
typedef struct {
    /**
     * The name of the mode
     */
    char name[PBIO_IODEV_MODE_NAME_SIZE + 1];
    /**
     * The number of data values for this mode.
     */
    uint8_t num_values;
    /**
     * The binary format of the data for this mode.
     */
    pbio_iodev_data_type_t data_type;
    /**
     * The number of digits for display, including the decimal point.
     */
    uint8_t digits;
    /**
     * The number of digits after the decimal point.
     */
    uint8_t decimals;
    /**
     * The minimum raw data value. This is just used for scaling an may not
     * actually be the minimum possible value.
     */
    float raw_min;
    /**
     * The maximum raw data value. This is just used for scaling an may not
     * actually be the maximum possible value.
     */
    float raw_max;
    /**
     * The minimum percent data value. This will be either 0.0 or -100.0.
     */
    float pct_min;
    /**
     * The maximum percent data value. This will always be 100.0.
     */
    float pct_max;
    /**
     * The minimum scaled data value. This is just used for scaling an may not
     * actually be the minimum possible value.
     */
    float si_min;
    /**
     * The maximum scaled data value. This is just used for scaling an may not
     * actually be the maximum possible value.
     */
    float si_max;
    /**
     * The units of measurement.
     */
    char uom[PBIO_IODEV_UOM_SIZE + 1];
} pbio_iodev_mode_t;

/**
 * Device-specific data for an I/O device.
 */
typedef struct {
    /**
     * The type identifier for the device.
     */
    pbio_iodev_type_id_t type_id;
    /**
     * The number of modes the device has. (Indicates size of *mode_info*).
     */
    uint8_t num_modes;
    /**
     * The number of "view" modes the device has. This modes are show in the
     * port monitor. Other modes may or may not be useable depending on the
     * device (e.g. some are for factory calibration).
     */
    uint8_t num_view_modes;
    /**
     * Array of mode info for all modes. Array size depends on the device.
     */
    pbio_iodev_mode_t mode_info[0];
} pbio_iodev_info_t;

typedef struct {
    /**
     * Pointer to the mode info for this device.
     */
    pbio_iodev_info_t *info;
    /**
     * The port the device is attached to.
     */
    pbio_port_t port;
    /**
     * The current active mode.
     */
    uint8_t mode;
    /**
     * Most recent binary data read from the device. How to interpret this data
     * is determined by the ::pbio_iodev_mode_t info associated with the current
     * *mode* of the device.
     */
    uint8_t bin_data[32];
} pbio_iodev_t;

#endif // _PBIO_IODEV_H_