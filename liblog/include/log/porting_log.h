
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

#ifndef LOG_TAG
#define LOG_TAG "TAG"
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

#define LOG_WARN ANDROID_LOG_WARN
#define LOG_VERBOSE ANDROID_LOG_VERBOSE

typedef void (*porting_log_callback_type)(
        android_LogPriority a_severity,
        const char* a_tag,
        const char* a_fileName,
        const char* a_funcName,
        unsigned int a_lineNr,
        const char* a_pStr
        );

LIBLOG_EXPORT void __set_porting_log_callback( porting_log_callback_type a_cb );

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

#ifndef ALOGV
#define ALOGV(str, ...) __log_format(ANDROID_LOG_VERBOSE, LOG_TAG, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)
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

#define ALOG(level, tag, str, ...) __log_format(level, tag, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__)

#ifndef __android_log_print
#define __android_log_print( level, tag, str, ... ) __log_format(level, tag, __FILE__, __FUNCTION__, __LINE__, str, ##__VA_ARGS__) 
#endif

#ifndef android_errorWriteLog
#define android_errorWriteLog( number, info_str ) __log_error_stamp(__FILE__, __FUNCTION__, __LINE__, number, info_str)
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

#define LOG_ALWAYS_FATAL_IF(con, ...) LOG_ALWAYS_IF( (con), ANDROID_LOG_FATAL, #con, ##__VA_ARGS__ )
#define ALOG_ASSERT(con, str, ...) LOG_ALWAYS_IF(!(con), ANDROID_LOG_FATAL, str, ##__VA_ARGS__ )
#define LOG_ALWAYS_FATAL(str, ...) LOG_ALWAYS_FATAL_IF(true, str, ##__VA_ARGS__ )
#define LOG_FATAL_IF(con,...) LOG_ALWAYS_FATAL_IF( con,##__VA_ARGS__ )
#define ALOGW_IF(con, str, ...) LOG_ALWAYS_IF(con, ANDROID_LOG_WARN, str, ##__VA_ARGS__ )


#ifdef __cplusplus
}
#endif

#endif

