#include <libcc/time.h>
#include <libcc/thread.h>

/* The first (low-resolution) ticks value of the application */
static uint64_t tick_start = 0;
static _cc_once_t tick_once = _CC_ONCE_INIT_;

static uint32_t tick_numerator_ns;
static uint32_t tick_denominator_ns;
static uint32_t tick_numerator_ms;
static uint32_t tick_denominator_ms;

static uint32_t _calculate_gcd(uint32_t a, uint32_t b) {
    if (b == 0) {
        return a;
    }
    return _calculate_gcd(b, (a % b));
}

_CC_API_PRIVATE(void) _tick_init(void) {
    uint64_t tick_freq;
    uint32_t gcd;

    tick_freq = _cc_query_performance_frequency();
    _cc_assert(tick_freq > 0 && tick_freq <= (uint64_t)0xFFFFFFFFu);

    gcd = _calculate_gcd(_CC_NS_PER_SECOND_, (uint32_t)tick_freq);
    tick_numerator_ns = (_CC_NS_PER_SECOND_ / gcd);
    tick_denominator_ns = (uint32_t)(tick_freq / gcd);

    gcd = _calculate_gcd(_CC_MS_PER_SECOND_, (uint32_t)tick_freq);
    tick_numerator_ms = (_CC_MS_PER_SECOND_ / gcd);
    tick_denominator_ms = (uint32_t)(tick_freq / gcd);

    tick_start = _cc_query_performance_counter();
    if (!tick_start) {
        --tick_start;
    }
}

_CC_API_PUBLIC(uint64_t) _cc_get_ticks_ns(void) {
    uint64_t starting_value, value;

    _cc_once(&tick_once,_tick_init);

    value = _cc_query_performance_counter();
    starting_value = (value - tick_start);
    value = (starting_value * tick_numerator_ns);
    _cc_assert(value >= starting_value);
    value /= tick_denominator_ns;
    return value;
}

/*
 * The elapsed time is stored as a uint32_t value. Therefore, the time will wrap
 * around to zero if the system is run continuously for 49.7 days.
 */
_CC_API_PUBLIC(uint64_t) _cc_get_ticks(void) {
    uint64_t starting_value, value;

    _cc_once(&tick_once,_tick_init);

    value = _cc_query_performance_counter();
    starting_value = (value - tick_start);
	_cc_assert(0 < tick_numerator_ms);
    value = (starting_value * tick_numerator_ms);
	_cc_assert(value >= starting_value);
    value /= tick_denominator_ms;
    return value;
}