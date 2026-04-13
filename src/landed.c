#include "landed.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "gpio.h"
#include "gps.h"
#include "log.h"
#include "radio_internal.h"
#include "drivers/gps/gps_driver.h"
#include "drivers/hal/usart_gps.h"
#include "drivers/hal/delay.h"

#ifdef RS41
#include "drivers/si4032/si4032.h"
#endif
#ifdef DFM17
#include "drivers/si4063/si4063.h"
#endif

#if LANDED_MODE_ENABLE

/* Maximum number of radio schedule entries we can save/restore.  The real
 * entry count is set at radio_init() time; this upper bound is simply large
 * enough to cover any practical configuration. */
#define LANDED_MAX_SCHEDULE_ENTRIES 32

static landed_state state;
static bool         descended;                /* altitude has fallen below arm threshold */
static uint32_t     stationary_counter;       /* seconds of near-zero velocity */
static uint32_t     next_wake_target_ms;      /* HAL_GetTick() value at which next wake should occur (absolute) */
static uint32_t     fix_wait_counter;         /* seconds waiting for GPS fix in ACQUIRING */
static uint32_t     transmit_counter;         /* seconds spent in TRANSMITTING (safety timeout) */
static uint32_t     pip_counter;              /* seconds until next PIP while SLEEPING */
static uint32_t     pipping_counter;          /* seconds spent in PIPPING */
static landed_state pip_return_state;         /* state to return to after PIPPING completes */
static uint16_t     ok_packets_at_wake;       /* snapshot of GPS ok_packets at wake; used to detect whether new data has arrived */
#if LANDED_MODE_TEST_SECONDS > 0
static uint32_t     test_counter;             /* seconds of uptime in INACTIVE (bench test) */
#endif

static int32_t      landing_lat;              /* geofence center, 1e-7 deg */
static int32_t      landing_lon;

/* Saved schedule state, used to restore original enabled/transmit_count. */
static bool         schedule_saved = false;
static bool         saved_enabled[LANDED_MAX_SCHEDULE_ENTRIES];
static uint8_t      saved_transmit_count[LANDED_MAX_SCHEDULE_ENTRIES];

/* --------------------------------------------------------------------------
 * Distance calculation (equirectangular approximation, integer math).
 * Accurate to ~1% for distances up to several km at typical latitudes -
 * more than sufficient for a recovery geofence.
 * -------------------------------------------------------------------------- */

/* cos(deg) * 10000, indexed by whole degrees 0..90. */
static const uint16_t cos_table_10000[91] = {
    10000, 9998, 9994, 9986, 9976, 9962, 9945, 9925, 9903, 9877,
     9848, 9816, 9781, 9744, 9703, 9659, 9613, 9563, 9511, 9455,
     9397, 9336, 9272, 9205, 9135, 9063, 8988, 8910, 8829, 8746,
     8660, 8572, 8480, 8387, 8290, 8192, 8090, 7986, 7880, 7771,
     7660, 7547, 7431, 7314, 7193, 7071, 6947, 6820, 6691, 6561,
     6428, 6293, 6157, 6018, 5878, 5736, 5592, 5446, 5299, 5150,
     5000, 4848, 4695, 4540, 4384, 4226, 4067, 3907, 3746, 3584,
     3420, 3256, 3090, 2924, 2756, 2588, 2419, 2250, 2079, 1908,
     1736, 1564, 1392, 1219, 1045,  872,  698,  523,  349,  175,
        0
};

/* Compute squared distance in meters between two lat/lon points.
 * Coordinates are degrees * 1e7 (as stored in gps_data).
 * Returns distance^2 in m^2 (avoids sqrt for geofence comparison). */
static uint64_t distance_squared_m2(int32_t lat1_e7, int32_t lon1_e7,
                                    int32_t lat2_e7, int32_t lon2_e7)
{
    /* 1 degree latitude ~= 111320 meters. */
    int64_t dlat_e7 = (int64_t)lat2_e7 - (int64_t)lat1_e7;
    int64_t dlon_e7 = (int64_t)lon2_e7 - (int64_t)lon1_e7;

    /* Convert delta degrees*1e7 to meters.  Use int64 to avoid overflow.
     * meters = delta_e7 * 111320 / 1e7 = delta_e7 * 11132 / 1e6 */
    int64_t dlat_m = (dlat_e7 * 11132) / 1000000;

    /* Longitude meters are scaled by cos(latitude).  Use midpoint latitude. */
    int32_t mid_lat_deg = (int32_t)(((int64_t)lat1_e7 + (int64_t)lat2_e7) / 2 / 10000000);
    if (mid_lat_deg < 0) mid_lat_deg = -mid_lat_deg;
    if (mid_lat_deg > 90) mid_lat_deg = 90;
    uint16_t cos_e4 = cos_table_10000[mid_lat_deg];

    int64_t dlon_m_raw = (dlon_e7 * 11132) / 1000000;
    int64_t dlon_m = (dlon_m_raw * (int64_t)cos_e4) / 10000;

    return (uint64_t)(dlat_m * dlat_m) + (uint64_t)(dlon_m * dlon_m);
}

/* --------------------------------------------------------------------------
 * Radio schedule save/restore helpers
 * -------------------------------------------------------------------------- */

static void schedule_save_if_needed(void)
{
    if (schedule_saved) return;
    uint8_t n = radio_transmit_entry_count;
    if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
    for (uint8_t i = 0; i < n; i++) {
        saved_enabled[i]        = radio_transmit_schedule[i].enabled;
        saved_transmit_count[i] = radio_transmit_schedule[i].transmit_count;
    }
    schedule_saved = true;
}

static void schedule_restore(void)
{
    if (!schedule_saved) return;
    uint8_t n = radio_transmit_entry_count;
    if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
    for (uint8_t i = 0; i < n; i++) {
        radio_transmit_schedule[i].enabled        = saved_enabled[i];
        radio_transmit_schedule[i].transmit_count = saved_transmit_count[i];
    }
    schedule_saved = false;
}

/* Configure schedule for TRANSMITTING: keep all entries enabled, but reduce
 * transmit_count to 1 so each mode sends a single message per visit.  The
 * LANDED_MODE_TRANSMIT_SECONDS safety window still bounds total duration. */
static void schedule_set_transmit_once(void)
{
    schedule_save_if_needed();
    uint8_t n = radio_transmit_entry_count;
    if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
    for (uint8_t i = 0; i < n; i++) {
        if (radio_transmit_schedule[i].transmit_count > 1 && radio_transmit_schedule[i].data_mode != RADIO_DATA_MODE_PIP) {
            radio_transmit_schedule[i].transmit_count = 1;
        }
    }
}

/* Configure schedule for PIPPING: disable every entry except PIP modes so the
 * normal main loop only fires a PIP. */
static void schedule_set_pip_only(void)
{
    schedule_save_if_needed();
    uint8_t n = radio_transmit_entry_count;
    if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
    for (uint8_t i = 0; i < n; i++) {
        if (radio_transmit_schedule[i].data_mode == RADIO_DATA_MODE_PIP) {
            /* Force-enable PIP entries (dedicated landed-mode PIP entry starts disabled). */
            radio_transmit_schedule[i].enabled = true;
        } else {
            radio_transmit_schedule[i].enabled = false;
        }
    }
}

/* Begin a PIP cycle: configure the schedule for PIP-only, clear pass_completed
 * on PIP entries, and transition into PIPPING.  Caller is responsible for
 * setting `pip_return_state` to the state to resume after the cycle. */
static void start_pipping(void)
{
    schedule_set_pip_only();
    uint8_t n = radio_transmit_entry_count;
    if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
    for (uint8_t i = 0; i < n; i++) {
        if (radio_transmit_schedule[i].data_mode == RADIO_DATA_MODE_PIP) {
            radio_transmit_schedule[i].pass_completed = false;
            radio_transmit_schedule[i].current_transmit_index = 0;
        }
    }
    state           = LANDED_STATE_PIPPING;
    pipping_counter = 0;
}

/* Transition to SLEEPING with an absolute-time-anchored wake target.  Pass
 * first_time=true on the very first SLEEPING entry (from STABILIZING) to
 * anchor the target to "now + SLEEP_SECONDS".  Subsequent entries advance
 * the existing target by one period, so the wake cadence stays locked to
 * absolute time even if TX or ACQUIRE took longer than expected.  If the
 * new target is already in the past (we fell more than a full period
 * behind), resync to now to avoid waking immediately on every cycle. */
static void enter_sleeping(bool first_time)
{
    uint32_t now = HAL_GetTick();
    if (first_time) {
        next_wake_target_ms = now + LANDED_MODE_SLEEP_SECONDS * 1000U;
    } else {
        next_wake_target_ms += LANDED_MODE_SLEEP_SECONDS * 1000U;
        if ((int32_t)(now - next_wake_target_ms) > 0) {
            next_wake_target_ms = now + LANDED_MODE_SLEEP_SECONDS * 1000U;
        }
    }
    state       = LANDED_STATE_SLEEPING;
    pip_counter = LANDED_MODE_PIP_INTERVAL_SECONDS;
}

/* --------------------------------------------------------------------------
 * Radio power control
 * -------------------------------------------------------------------------- */

static void radio_inhibit(void)
{
#ifdef RS41
    si4032_disable_tx();
#endif
#ifdef DFM17
    si4063_inhibit_tx();
#endif
}

/* --------------------------------------------------------------------------
 * GPS power control
 * -------------------------------------------------------------------------- */

static void gps_sleep(void)
{
    log_info("LANDED: Putting GPS into backup mode\n");
    gps_driver_sleep();
}

/* Wake the GPS from backup mode.  The M10 wakes on UART RX activity, but
 * a short low pulse (single byte's start bit) is unreliable — observed
 * ~50% wake rate.  Use a true UART BREAK frame: continuous low for ~10-11
 * bit times (≈290 µs at 38400) with no intervening high transitions.  This
 * is a much stronger and more deterministic wake signal.
 *
 * NOTE: we deliberately do NOT call gps_driver_clear_data() here.  Keeping
 * the prior fix data in place lets us fall back to the last-known-good
 * position if the chip fails to wake.  ACQUIRING distinguishes fresh data
 * from stale by snapshotting ok_packets at wake time and comparing. */
static void gps_wake(void)
{
    log_info("LANDED: Waking GPS from backup mode\n");
    gps_driver_reset_parser();
    /* Send several break frames spaced apart, so we get repeated long-low
     * pulses with high gaps the chip can latch on to. */
    for (int i = 0; i < 20; i++) {
        usart_gps_send_break();
        delay_ms(20);
    }
    /* Follow up with a few 0x00 bytes for additional UART activity in case
     * the chip needs more than just break to fully resume. */
    for (int i = 0; i < 30; i++) {
        usart_gps_send_byte(0x00);
        delay_ms(10);
    }
    /* Allow the receiver to fully resume operation. */
    delay_ms(200);
}

/* --------------------------------------------------------------------------
 * Geofence check
 * -------------------------------------------------------------------------- */

static bool geofence_violated(const gps_data *gps)
{
#if LANDED_MODE_GEOFENCE_RADIUS_METERS <= 0
    (void)gps;
    return false;
#else
    if (!GPS_HAS_FIX(*gps)) {
        /* Can't judge without a fix; don't violate. */
        return false;
    }
    uint64_t d2 = distance_squared_m2(landing_lat, landing_lon,
                                      gps->latitude_degrees_10000000,
                                      gps->longitude_degrees_10000000);
    uint64_t r  = (uint64_t)LANDED_MODE_GEOFENCE_RADIUS_METERS;
    return d2 > (r * r);
#endif
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void landed_init(void)
{
    state               = LANDED_STATE_INACTIVE;
    descended           = false;
    stationary_counter  = 0;
    next_wake_target_ms = 0;
    fix_wait_counter    = 0;
    transmit_counter    = 0;
    pip_counter         = 0;
    pipping_counter     = 0;
    pip_return_state    = LANDED_STATE_SLEEPING;
    ok_packets_at_wake  = 0;
    landing_lat         = 0;
    landing_lon         = 0;
    schedule_saved      = false;
#if LANDED_MODE_TEST_SECONDS > 0
    test_counter        = 0;
#endif
    log_info("LANDED: Module initialized\n");
}

landed_state landed_get_state(void) { return state; }

bool landed_is_active(void)
{
    return state != LANDED_STATE_INACTIVE && state != LANDED_STATE_ARMED;
}

static inline bool velocity_stationary(const gps_data *gps)
{
    int32_t climb = gps->climb_cm_per_second;
    if (climb < 0) climb = -climb;
    return (climb < LANDED_MODE_CLIMB_THRESHOLD_CM_S)
        && (gps->ground_speed_cm_per_second < LANDED_MODE_SPEED_THRESHOLD_CM_S);
}

bool landed_update(gps_data *gps)
{
    int32_t altitude_m = gps->altitude_mm / 1000;

    switch (state) {

    case LANDED_STATE_INACTIVE:
#if LANDED_MODE_TEST_SECONDS > 0
        test_counter++;
        if (test_counter >= LANDED_MODE_TEST_SECONDS) {
            log_info("LANDED: Bench test timer expired; entering STABILIZING\n");
            descended = true;
            state              = LANDED_STATE_STABILIZING;
            stationary_counter = 1;
            return true;
        }
#endif
        if (GPS_HAS_FIX(*gps) && altitude_m > LANDED_MODE_ARM_ALTITUDE_METERS) {
            log_info("LANDED: Armed at %ld m\n", altitude_m);
            state     = LANDED_STATE_ARMED;
            descended = false;
        }
        return true;

    case LANDED_STATE_ARMED:
        /* Track whether we have descended back below the arm threshold. */
        if (GPS_HAS_FIX(*gps)) {
            if (altitude_m < LANDED_MODE_ARM_ALTITUDE_METERS) {
                if (!descended) {
                    log_info("LANDED: Descended below arm altitude\n");
                    descended = true;
                }
            } else if (descended) {
                log_info("LANDED: Re-ascended above arm altitude\n");
                descended = false;
            }

            if (descended && velocity_stationary(gps)) {
                state = LANDED_STATE_STABILIZING;
                stationary_counter = 1;
            }
        }
        return true;

    case LANDED_STATE_STABILIZING:
#if LANDED_MODE_TEST_SECONDS > 0
        /* Bench test: skip GPS/velocity checks to allow indoor testing. */
#else
        if (!GPS_HAS_FIX(*gps) || !velocity_stationary(gps)) {
            state = LANDED_STATE_ARMED;
            stationary_counter = 0;
            return true;
        }
#endif
        stationary_counter++;
        if (stationary_counter >= LANDED_MODE_STATIONARY_SECONDS) {
            /* Latch landing position as the (possibly new) geofence center. */
            landing_lat = gps->latitude_degrees_10000000;
            landing_lon = gps->longitude_degrees_10000000;
            log_info("LANDED: Landing confirmed; entering sleep\n");
            gps_sleep();
            radio_inhibit();
            enter_sleeping(true);
        }
        return true;

    case LANDED_STATE_SLEEPING:
    {
        bool wake_due = (int32_t)(HAL_GetTick() - next_wake_target_ms) >= 0;
#if LANDED_MODE_PIP_ENABLE
        if (pip_counter > 0) pip_counter--;
        if (pip_counter == 0 && !wake_due) {
            /* Trigger a PIP without waking the GPS. */
            pip_return_state = LANDED_STATE_SLEEPING;
            start_pipping();
            return true;
        }
#endif
        if (wake_due) {
            /* Snapshot ok_packets BEFORE wake so we can detect whether the
             * chip actually responded (new packets arriving = ok_packets
             * increases past this snapshot).  If it stays equal, the wake
             * failed and we'll fall back to the cached prior fix. */
            ok_packets_at_wake = gps->ok_packets;
            gps_wake();
            state            = LANDED_STATE_ACQUIRING;
            fix_wait_counter = 0;
            pip_counter      = LANDED_MODE_PIP_INTERVAL_SECONDS;
        }
    }
        return false;

    case LANDED_STATE_PIPPING:
        pipping_counter++;
        /* Check if any PIP entry has completed its full pass of
         * RADIO_TX_PIP_COUNT transmissions.  The pass_completed flag is set
         * by the radio state machine when current_transmit_index wraps to 0
         * and persists until we clear it — no timing window to miss. */
        {
            uint8_t n = radio_transmit_entry_count;
            if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
            for (uint8_t i = 0; i < n; i++) {
                /* Note: do NOT check `enabled` here — radio_next_transmit_entry()
                 * auto-disables the entry at the same moment it sets pass_completed
                 * during PIPPING, so the two flags are never both true together. */
                if (radio_transmit_schedule[i].data_mode == RADIO_DATA_MODE_PIP
                    && radio_transmit_schedule[i].pass_completed) {
                    schedule_restore();
                    /* Only inhibit the radio if we're returning to SLEEPING.
                     * If returning to ACQUIRING, the GPS is still awake and
                     * we keep the radio idle but available for the next PIP. */
                    if (pip_return_state == LANDED_STATE_SLEEPING) {
                        radio_inhibit();
                    }
                    pip_counter   = LANDED_MODE_PIP_INTERVAL_SECONDS;
                    state         = pip_return_state;
                    return false;
                }
            }
        }
        if (pipping_counter >= 30) {
            /* Safety timeout. */
            log_info("LANDED: PIP safety timeout\n");
            schedule_restore();
            if (pip_return_state == LANDED_STATE_SLEEPING) {
                radio_inhibit();
            }
            pip_counter   = LANDED_MODE_PIP_INTERVAL_SECONDS;
            state         = pip_return_state;
            return false;
        }
        return true;

    case LANDED_STATE_ACQUIRING:
    {
        fix_wait_counter++;

        /* "gps_responding" = ok_packets has advanced past the snapshot taken
         * at wake time, meaning the chip actually woke and is sending data.
         * If it stays equal, the chip never woke. */
        bool gps_responding = (gps->ok_packets > ok_packets_at_wake);
        bool have_fix       = GPS_HAS_FIX(*gps);

        if (gps_responding && have_fix) {
            /* Fresh, current fix from this wake cycle — normal path. */
            if (geofence_violated(gps)) {
                log_info("LANDED: Geofence violated; returning to ARMED\n");
                schedule_restore();
                state     = LANDED_STATE_ARMED;
                descended = false;  /* unknown altitude relationship */
                return true;
            }
            log_info("LANDED: Starting transmit cycle (fresh fix)\n");
            schedule_set_transmit_once();
            /* Clear pass_completed on all enabled entries so we detect
             * when each one finishes its single transmission. */
            {
                uint8_t n = radio_transmit_entry_count;
                if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
                for (uint8_t i = 0; i < n; i++) {
                    radio_transmit_schedule[i].pass_completed = false;
                    radio_transmit_schedule[i].current_transmit_index = 0;
                }
            }
            state            = LANDED_STATE_TRANSMITTING;
            transmit_counter = 0;
            return true;
        }

        if (fix_wait_counter >= LANDED_MODE_GPS_FIX_TIMEOUT_SECONDS) {
            if (!gps_responding && have_fix) {
                /* Wake failed, but the prior fix is still in the gps_data
                 * struct (we do not clear on wake).  Transmit it as a
                 * cached fallback — better than silence.  Skip geofence:
                 * cached lat/lon is by definition near the landing site. */
                log_info("LANDED: GPS wake failed; transmitting cached position\n");
                schedule_set_transmit_once();
                {
                    uint8_t n = radio_transmit_entry_count;
                    if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
                    for (uint8_t i = 0; i < n; i++) {
                        radio_transmit_schedule[i].pass_completed = false;
                        radio_transmit_schedule[i].current_transmit_index = 0;
                    }
                }
                state            = LANDED_STATE_TRANSMITTING;
                transmit_counter = 0;
                return true;
            }
            /* No usable data at all — silent sleep, retry next cycle. */
            log_info("LANDED: GPS fix timeout; no usable data, returning to sleep\n");
            gps_sleep();
            radio_inhibit();
            enter_sleeping(false);
            return false;
        }
#if LANDED_MODE_PIP_ENABLE
        if (pip_counter > 0) pip_counter--;
        if (pip_counter == 0) {
            /* Fire a PIP while still waiting for fix. */
            pip_return_state = LANDED_STATE_ACQUIRING;
            start_pipping();
            return true;
        }
#endif
        return false;
    }

    case LANDED_STATE_TRANSMITTING:
        transmit_counter++;
        /* Detect completion: each enabled entry has transmit_count=1.
         * The radio sets pass_completed when current_transmit_index wraps
         * to 0 after finishing a pass.  Once every enabled entry has its
         * flag set, the full cycle is done. */
        {
            uint8_t n = radio_transmit_entry_count;
            if (n > LANDED_MAX_SCHEDULE_ENTRIES) n = LANDED_MAX_SCHEDULE_ENTRIES;
            bool all_done = true;
            for (uint8_t i = 0; i < n; i++) {
                if (radio_transmit_schedule[i].enabled
                    && !radio_transmit_schedule[i].pass_completed) {
                    all_done = false;
                    break;
                }
            }
            if (all_done && transmit_counter > 1) {
                log_info("LANDED: Transmit cycle complete; back to sleep\n");
                schedule_restore();
                gps_sleep();
                radio_inhibit();
                enter_sleeping(false);
                return false;
            }
        }
        if (transmit_counter >= LANDED_MODE_TRANSMIT_SECONDS) {
            log_info("LANDED: Transmit safety timeout; back to sleep\n");
            schedule_restore();
            gps_sleep();
            radio_inhibit();
            enter_sleeping(false);
            return false;
        }
        return true;
    }

    /* Unreachable */
    return true;
}

#else   /* !LANDED_MODE_ENABLE */

void         landed_init(void)                 { }
landed_state landed_get_state(void)            { return LANDED_STATE_INACTIVE; }
bool         landed_is_active(void)            { return false; }
bool         landed_update(gps_data *gps)      { (void)gps; return true; }

#endif  /* LANDED_MODE_ENABLE */
