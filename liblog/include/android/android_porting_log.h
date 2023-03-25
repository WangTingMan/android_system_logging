#pragma once
#include <stdint.h>
#include <log/porting_log.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Identifies a specific log buffer for __android_log_buf_write()
 * and __android_log_buf_print().
 */
typedef enum log_id
{
    LOG_ID_MIN = 0,

    /** The main log buffer. This is the only log buffer available to apps. */
    LOG_ID_MAIN = 0,
    /** The radio log buffer. */
    LOG_ID_RADIO = 1,
    /** The event log buffer. */
    LOG_ID_EVENTS = 2,
    /** The system log buffer. */
    LOG_ID_SYSTEM = 3,
    /** The crash log buffer. */
    LOG_ID_CRASH = 4,
    /** The statistics log buffer. */
    LOG_ID_STATS = 5,
    /** The security log buffer. */
    LOG_ID_SECURITY = 6,
    /** The kernel log buffer. */
    LOG_ID_KERNEL = 7,

    LOG_ID_MAX,

    /** Let the logging function choose the best log target. */
    LOG_ID_DEFAULT = 0x7FFFFFFF
} log_id_t;

/**
 * Logger data struct used for writing log messages to liblog via __android_log_write_logger_data()
 * and sending log messages to user defined loggers specified in __android_log_set_logger().
 */
struct __android_log_message
{
    /** Must be set to sizeof(__android_log_message) and is used for versioning. */
    size_t struct_size;

    /** {@link log_id_t} values. */
    int32_t buffer_id;

    /** {@link android_LogPriority} values. */
    int32_t priority;

    /** The tag for the log message. */
    const char* tag;

    /** Optional file name, may be set to nullptr. */
    const char* file;

    /** Optional line number, ignore if file is nullptr. */
    uint32_t line;

    /** The log message itself. */
    const char* message;
};

/**
 * Prototype for the 'logger' function that is called for every log message.
 */
typedef void ( *__android_logger_function )( const struct __android_log_message* log_message );

/**
 * Prototype for the 'abort' function that is called when liblog will abort due to
 * __android_log_assert() failures.
 */
typedef void ( *__android_aborter_function )( const char* abort_message );

/**
 * Writes the log message to logd.  This is an __android_logger_function and can be provided to
 * __android_log_set_logger().  It is the default logger when running liblog on a device.
 *
 * @param log_message the log message to write, see __android_log_message.
 *
 * Available since API level 30.
 */
LIBLOG_EXPORT void __android_log_logd_logger( const struct __android_log_message* log_message );

/**
 * Writes a formatted string to log buffer `id`,
 * with priority `prio` and tag `tag`.
 * The details of formatting are the same as for
 * [printf(3)](http://man7.org/linux/man-pages/man3/printf.3.html).
 *
 * Apps should use __android_log_print() instead.
 */
LIBLOG_EXPORT int __android_log_buf_print( int bufID, int prio, const char* tag, const char* fmt, ... );

/**
 * Sets a user defined logger function.  All log messages sent to liblog will be set to the
 * function pointer specified by logger for processing.  It is not expected that log messages are
 * already terminated with a new line.  This function should add new lines if required for line
 * separation.
 *
 * @param logger the new function that will handle log messages.
 *
 * Available since API level 30.
 */
LIBLOG_EXPORT void __android_log_set_logger( __android_logger_function logger );

/**
 * Sets a user defined aborter function that is called for __android_log_assert() failures.  This
 * user defined aborter function is highly recommended to abort and be noreturn, but is not strictly
 * required to.
 *
 * @param aborter the new aborter function, see __android_aborter_function.
 *
 * Available since API level 30.
 */
LIBLOG_EXPORT void __android_log_set_aborter( __android_aborter_function aborter );

/**
 * Calls the stored aborter function.  This allows for other logging libraries to use the same
 * aborter function by calling this function in liblog.
 *
 * @param abort_message an additional message supplied when aborting, for example this is used to
 *                      call android_set_abort_message() in __android_log_default_aborter().
 *
 * Available since API level 30.
 */
LIBLOG_EXPORT void __android_log_call_aborter( const char* abort_message );

/**
 * Writes the constant string `text` to the log, with priority `prio` and tag
 * `tag`.
 */
LIBLOG_EXPORT int __android_log_write(int prio, const char* tag, const char* text);

#ifdef __cplusplus
}
#endif

