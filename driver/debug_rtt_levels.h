/**
 * @file debug_rtt_levels.h
 * @brief RTT Debug Output with Log Levels (DEBUG, INFO, OK, WARN, ERROR)
 * @date 2026-06-25
 *
 * Provides formatted debug output functions for SEGGER RTT with severity levels:
 * - rtt_debug() - Development debugging information [DEBUG]
 * - rtt_info()  - Informational messages [INFO]
 * - rtt_ok()    - Success/OK status [OK]
 * - rtt_warn()  - Warning messages [WARN]
 * - rtt_error() - Error messages [ERROR]
 *
 * Usage:
 *   rtt_ok("Device initialized\r\n");
 *   rtt_error("Init failed with code %d\r\n", errno);
 *   rtt_warn("Register write took %u ms\r\n", duration);
 *   rtt_info("Counter value: %lu\r\n", counter);
 *   rtt_debug("Temp sensor raw: 0x%04x\r\n", raw_value);
 */

#ifndef DEBUG_RTT_LEVELS_H
#define DEBUG_RTT_LEVELS_H

#include <stdarg.h>
#include "debug.h"
#include "SEGGER_RTT.h"

// ==================== Base printf (no prefix) ====================

/**
 * @brief Plain formatted output to RTT (no prefix)
 * @param format Printf format string
 * @param ... Variable arguments
 *
 * Use for output that shouldn't have a level prefix
 */
static inline void rtt_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    (void)SEGGER_RTT_vprintf(0u, format, &args);
    va_end(args);
}

// ==================== Debug Level Functions ====================

/**
 * @brief Debug level message [DEBUG] - Development tracing
 * @param format Printf format string
 * @param ... Variable arguments
 *
 * Example: rtt_debug("Register read addr=0x%04x value=0x%04x\r\n", addr, value);
 * Output: [DEBUG] Register read addr=0x1234 value=0x5678
 */
static inline void rtt_debug(const char *format, ...)
{
    va_list args;
    DEBUG_writeString("[DEBUG] ");

    va_start(args, format);
    (void)SEGGER_RTT_vprintf(0u, format, &args);
    va_end(args);
}

/**
 * @brief Info level message [INFO] - Normal operation
 * @param format Printf format string
 * @param ... Variable arguments
 *
 * Example: rtt_info("Reading sensor every %u ms\r\n", interval);
 * Output: [INFO] Reading sensor every 1000 ms
 */
static inline void rtt_info(const char *format, ...)
{
    va_list args;
    DEBUG_writeString("[INFO] ");

    va_start(args, format);
    (void)SEGGER_RTT_vprintf(0u, format, &args);
    va_end(args);
}

/**
 * @brief OK/Success level message [OK] - Operation successful
 * @param format Printf format string
 * @param ... Variable arguments
 *
 * Example: rtt_ok("Device ID %d initialized successfully\r\n", device_id);
 * Output: [OK] Device ID 1 initialized successfully
 */
static inline void rtt_ok(const char *format, ...)
{
    va_list args;
    DEBUG_writeString("[OK] ");

    va_start(args, format);
    (void)SEGGER_RTT_vprintf(0u, format, &args);
    va_end(args);
}

/**
 * @brief Warning level message [WARN] - Potential issues
 * @param format Printf format string
 * @param ... Variable arguments
 *
 * Example: rtt_warn("ADC read took longer than expected: %u ms\r\n", time);
 * Output: [WARN] ADC read took longer than expected: 250 ms
 */
static inline void rtt_warn(const char *format, ...)
{
    va_list args;
    DEBUG_writeString("[WARN] ");

    va_start(args, format);
    (void)SEGGER_RTT_vprintf(0u, format, &args);
    va_end(args);
}

/**
 * @brief Error level message [ERROR] - Failure/error condition
 * @param format Printf format string
 * @param ... Variable arguments
 *
 * Example: rtt_error("EEPROM write failed: error code %d\r\n", error);
 * Output: [ERROR] EEPROM write failed: error code -2
 */
static inline void rtt_error(const char *format, ...)
{
    va_list args;
    DEBUG_writeString("[ERROR] ");

    va_start(args, format);
    (void)SEGGER_RTT_vprintf(0u, format, &args);
    va_end(args);
}

#endif /* DEBUG_RTT_LEVELS_H */
