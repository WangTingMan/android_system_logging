#include <android/android_porting_log.h>
#include <log/porting_log.h>

void __android_log_logd_logger( const struct __android_log_message* log_message )
{
    android_LogPriority level = ANDROID_LOG_INFO;
    switch( log_message->priority )
    {
        /** For internal use only.  */
    case ANDROID_LOG_UNKNOWN:
        break;
            /** The default priority, for internal use only.  */
    case ANDROID_LOG_DEFAULT: /* only for SetMinPriority() */
        break;
            /** Verbose logging. Should typically be disabled for a release apk. */
    case ANDROID_LOG_VERBOSE:
            break;
            /** Debug logging. Should typically be disabled for a release apk. */
    case ANDROID_LOG_DEBUG:
            break;
            /** Informational logging. Should typically be disabled for a release apk. */
    case ANDROID_LOG_INFO:
            break;
            /** Warning logging. For use with recoverable failures. */
    case ANDROID_LOG_WARN:
            break;
            /** Error logging. For use with unrecoverable failures. */
    case ANDROID_LOG_ERROR:
            break;
            /** Fatal logging. For use when aborting. */
    case ANDROID_LOG_FATAL:
            break;
            /** For internal use only.  */
    case ANDROID_LOG_SILENT: /* only for SetMinPriority(); must be last */
            break;
    default:
        break;
    }
}

