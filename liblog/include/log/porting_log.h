
#ifndef ANDROID_LOG_H_HEADER
#define ANDROID_LOG_H_HEADER

#if defined(WIN32) || defined(_MSC_VER)

#if defined(LIBLOG_IMPLEMENTATION)
#define LIBLOG_EXPORT __declspec(dllexport)
#else
#define LIBLOG_EXPORT __declspec(dllimport)
#endif  // defined(LIBLOG_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(LIBLOG_IMPLEMENTATION)
#define LIBLOG_EXPORT __attribute__((visibility("default")))
#else
#define LIBLOG_EXPORT
#endif  // defined(LIBLOG_IMPLEMENTATION)
#endif

#include <stdint.h>

/*
 * Event log entry types.
 */
typedef enum {
    /* Special markers for android_log_list_element type */
    EVENT_TYPE_LIST_STOP = '\n', /* declare end of list  */
    EVENT_TYPE_UNKNOWN = '?',    /* protocol error       */

    /* must match with declaration in java/android/android/util/EventLog.java */
    EVENT_TYPE_INT = 0,  /* int32_t */
    EVENT_TYPE_LONG = 1, /* int64_t */
    EVENT_TYPE_STRING = 2,
    EVENT_TYPE_LIST = 3,
    EVENT_TYPE_FLOAT = 4,
} AndroidEventLogType;

#ifndef LOG_TAG
#define LOG_TAG ""
#endif

#ifdef __cplusplus
extern "C" {
#endif

    enum android_LogPriority
    {
        /** For internal use only.  */
        ANDROID_LOG_UNKNOWN = 0,
        /** The default priority, for internal use only.  */
        ANDROID_LOG_DEFAULT, /* only for SetMinPriority() */
        /** Verbose logging. Should typically be disabled for a release apk. */
        ANDROID_LOG_VERBOSE,
        /** Debug logging. Should typically be disabled for a release apk. */
        ANDROID_LOG_DEBUG,
        /** Informational logging. Should typically be disabled for a release apk. */
        ANDROID_LOG_INFO,
        /** Warning logging. For use with recoverable failures. */
        ANDROID_LOG_WARN,
        /** Error logging. For use with unrecoverable failures. */
        ANDROID_LOG_ERROR,
        /** Fatal logging. For use when aborting. */
        ANDROID_LOG_FATAL,
        /** For internal use only.  */
        ANDROID_LOG_SILENT, /* only for SetMinPriority(); must be last */
    };

#ifndef LOG_VERBOSE
#define LOG_VERBOSE ANDROID_LOG_VERBOSE
#endif

#define LOG_ERR ANDROID_LOG_ERROR

typedef void (*porting_log_callback_type)(
        android_LogPriority a_severity,
        const char* a_tag,
        const char* a_fileName,
        const char* a_funcName,
        unsigned int a_lineNr,
        const char* a_pStr
        );

#define LIBLOG_LOG_CALLBACK_DEFINED
LIBLOG_EXPORT void __set_porting_log_callback( porting_log_callback_type a_cb );

LIBLOG_EXPORT void __set_default_log_file_name( const char* a_file_name );

LIBLOG_EXPORT int __log_format(
        android_LogPriority a_severity,
        const char* a_tag,
        const char* a_fileName,
        const char* a_funcName,
        unsigned int a_lineNr,
        const char* a_pStr,
        ... );

LIBLOG_EXPORT int __log_error_stamp(
    const char* a_fileName,
    const char* a_funcName,
    unsigned int a_lineNr,
    uint32_t a_number,
    const char* a_pStr
    );

LIBLOG_EXPORT int __android_log_error_write( int tag, const char* subTag, int32_t uid,
    const char* data, uint32_t dataLen, const char* file, uint32_t line );

LIBLOG_EXPORT int __assert_message( const char* file, int line, const char* failed_expression );

LIBLOG_EXPORT int ___syslog_message( int level, const char* file, int line, const char* a_pStr, ... );

/*
 * Use the per-tag properties "log.tag.<tagname>" to generate a runtime
 * result of non-zero to expose a log. prio is ANDROID_LOG_VERBOSE to
 * ANDROID_LOG_FATAL. default_prio if no property. Undefined behavior if
 * any other value.
 */
LIBLOG_EXPORT int __android_log_is_loggable(int prio, const char* tag, int default_prio);
LIBLOG_EXPORT int __android_log_is_loggable_len(int prio, const char* tag, size_t len, int default_prio);

#ifndef __assert
#define __assert __assert_message
#endif
#define syslog( level, str, ...) ___syslog_message(level, __FILE__, __LINE__, str, ##__VA_ARGS__)

#ifndef android_errorWriteWithInfoLog
#define android_errorWriteWithInfoLog(tag, subTag, uid, data, dataLen) \
  __android_log_error_write(tag, subTag, uid, data, dataLen, __FILE__, __LINE__)
#endif

#ifndef ALOGV
#define ALOGV(str, ...) __log_format(ANDROID_LOG_VERBOSE, LOG_TAG, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)
#endif

#ifndef ALOGI
#define ALOGI(str, ...) __log_format(ANDROID_LOG_INFO, LOG_TAG, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)
#endif

#ifndef ALOGI_LOCATION
#define ALOGI_LOCATION(str, file, line, ...) __log_format(ANDROID_LOG_INFO, LOG_TAG, file, __FUNCTION__, line, str, ##__VA_ARGS__)
#endif

#ifndef ALOGW
#define ALOGW(str, ...) __log_format(ANDROID_LOG_WARN, LOG_TAG, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)
#endif

#ifndef ALOGE
#define ALOGE(str, ...) __log_format(ANDROID_LOG_ERROR, LOG_TAG, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)
#endif

#ifndef ALOGD
#define ALOGD(str, ...) __log_format(ANDROID_LOG_DEBUG, LOG_TAG, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)
#endif

#ifndef ALOGF
#define ALOGF(str, ...) __log_format(ANDROID_LOG_FATAL, LOG_TAG, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)
#endif

#define ALOG(level, tag, str, ...) __log_format(level, tag, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)

#ifndef LOGWRAPPER
#define LOGWRAPPER(fmt, ...) ALOGD(fmt, ##__VA_ARGS__)
#endif

#ifndef __android_log_print
#define __android_log_print( level, tag, str, ... ) __log_format(level, tag, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__) 
#endif

#define __FAKE_USE_VA_ARGS(...) ((void)(0))

#ifndef android_errorWriteLog
#define android_errorWriteLog( number, info_str ) __log_error_stamp(__FILE__, __FUNCTION__, __LINE__, number, info_str)
#endif

#ifndef __predict_false
#define __predict_false(exp) (exp)
#endif

#define LOG_ALWAYS_IF(con, level, str, ...)         \
    {                                               \
        if( ( con ) )                               \
        {                                           \
            __log_format( level,                    \
                LOG_TAG, __FILE__, __FUNCTION__,    \
                __LINE__, str, ##__VA_ARGS__ );     \
        }                                           \
    }

#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(con, ...) LOG_ALWAYS_IF( (con), ANDROID_LOG_FATAL, #con, ##__VA_ARGS__ )
#endif

#ifndef ALOG_ASSERT
#define ALOG_ASSERT(con, str, ...) LOG_ALWAYS_IF(!(con), ANDROID_LOG_FATAL, str, ##__VA_ARGS__ )
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(str, ...) LOG_ALWAYS_IF(true, ANDROID_LOG_FATAL, str, ##__VA_ARGS__ )
#endif

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(con,...) LOG_ALWAYS_FATAL_IF( con,##__VA_ARGS__ )
#endif

#define ALOGW_IF(con, str, ...) LOG_ALWAYS_IF(con, ANDROID_LOG_WARN, str, ##__VA_ARGS__ )

#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...)                                                             \
  ((__predict_false(cond))                                                              \
       ? (__FAKE_USE_VA_ARGS(##__VA_ARGS__), (void)ALOG(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)) \
       : ((void)0))
#endif

#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...)                                                             \
  ((__predict_false(cond))                                                              \
       ? (__FAKE_USE_VA_ARGS(##__VA_ARGS__), (void)ALOG(ANDROID_LOG_DEBUG, LOG_TAG, ##__VA_ARGS__)) \
       : ((void)0))
#endif

#ifndef IF_ALOGV
#if LOG_NDEBUG
#define IF_ALOGV() if (false)
#else
#define IF_ALOGV() if (true)
#endif
#endif

#define android_testLog(prio, tag) true

/*
 * Conditional given a desired logging priority and tag.
 */
#ifndef IF_ALOG
#define IF_ALOG(priority, tag) if (android_testLog(ANDROID_##priority, tag))
#endif

#ifndef dprintf
#define dprintf(...)
#endif

#ifdef __cplusplus
}
#endif

#endif

