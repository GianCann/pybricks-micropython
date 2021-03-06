// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// System Supervisor

// This file monitors the system health and takes care of things like shutting
// down power on low battery an shutting down motors on overcurrent conditions.

#include <stdbool.h>

#include <pbdrv/reset.h>
#include <pbsys/status.h>

/**
 * Polls the system supervisor.
 *
 * This is called periodically to handle any changes in the system state.
 */
void pbsys_supervisor_poll() {
    // Shut down on low voltage so we don't damage rechargeable batteries
    if (pbsys_status_test_debounce(PBSYS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN, true, 3000)) {
        pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF);
    }
}
