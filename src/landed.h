#ifndef __LANDED_H
#define __LANDED_H

#include <stdbool.h>
#include <stdint.h>
#include "gps.h"

typedef enum _landed_state {
    LANDED_STATE_INACTIVE = 0,  // Feature disabled or altitude threshold not yet reached
    LANDED_STATE_ARMED,         // Altitude threshold exceeded; monitoring for landing
    LANDED_STATE_STABILIZING,   // Descended and velocity low; counting stationary seconds
    LANDED_STATE_SLEEPING,      // GPS + radio sleeping between wake cycles
    LANDED_STATE_PIPPING,       // Short PIP transmission while GPS remains asleep
    LANDED_STATE_ACQUIRING,     // GPS woken, waiting for fix
    LANDED_STATE_TRANSMITTING,  // One full pass of enabled transmit modes
} landed_state;

/* Initialize the landed mode module.  Safe to call even when disabled. */
void landed_init(void);

/* Called from the main loop approximately once per second with the latest GPS
 * data.  Returns true if the normal radio main loop should run on this tick
 * (INACTIVE, ARMED, STABILIZING, PIPPING, TRANSMITTING); returns false if the
 * landed module is handling everything (SLEEPING, ACQUIRING) and the caller
 * should idle. */
bool landed_update(gps_data *gps);

/* Accessor for the current state (useful for telemetry / debug output). */
landed_state landed_get_state(void);

/* Returns true whenever the module is outside INACTIVE/ARMED - i.e. it has
 * latched a landing and is actively managing the sleep/wake cycle. */
bool landed_is_active(void);

#endif
