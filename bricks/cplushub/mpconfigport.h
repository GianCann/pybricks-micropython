// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "stm32l431xx.h"

#define MICROPY_HW_BOARD_NAME           "LEGO TECHNIC Control+ Hub"
#define MICROPY_HW_MCU_NAME             "STM32L431RC"

#define PYBRICKS_HEAP_KB                16

// Pybricks modules
#define PYBRICKS_PY_ADVANCED            (1)
#define PYBRICKS_PY_BATTERY             (1)
#define PYBRICKS_PY_CPLUSHUB            (1)
#define PYBRICKS_PY_DEBUG               (1)
#define PYBRICKS_PY_MOTOR               (1)
#define PYBRICKS_PY_PARAMETERS          (1)
#define PYBRICKS_PY_PUPDEVICES          (1)
#define PYBRICKS_PY_TOOLS               (1)

#define MICROPY_ENABLE_COMPILER         (1)
#define MICROPY_FLOAT_IMPL              (MICROPY_FLOAT_IMPL_FLOAT)

#include "../stm32configport.h"