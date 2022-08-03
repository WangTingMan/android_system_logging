#include <android/android_porting_log.h>
#include <log/porting_log.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>

static __android_aborter_function s_aborter = nullptr;
static __android_logger_function s_logger_function = nullptr;

#ifndef LOGLINE_SIZE
#define LOGLINE_SIZE 1024
#endif

void __android_log_logd_logger( const struct __android_log_message* log_message )
{
    android_LogPriority level = static_cast< android_LogPriority >( log_message->priority );
    __log_format( level, log_message->tag,
        log_message->file, __FUNCTION__, log_message->line,
        "%s", log_message->message );
}

int __android_log_buf_print( int bufID, int prio, const char* tag, const char* fmt, ... )
{
    android_LogPriority level = static_cast< android_LogPriority >( prio );
    char pBuffer[LOGLINE_SIZE];
    va_list vaList;
    va_start( vaList, fmt );
    ( void )vsnprintf( pBuffer, LOGLINE_SIZE - 1, fmt, vaList );
    va_end( vaList );

    __log_format( level, tag,
        nullptr, __FUNCTION__, 0,
        "%s", pBuffer );
    return 0;
}

void __android_log_set_aborter( __android_aborter_function aborter )
{
    s_aborter = aborter;
}

void __android_log_set_logger( __android_logger_function logger )
{
    s_logger_function = logger;
}

void __android_log_call_aborter( const char* abort_message )
{
    __log_format( ANDROID_LOG_ERROR, "",
        nullptr, __FUNCTION__, 0,
        "%s", abort_message );

    if( s_aborter )
    {
        s_aborter( abort_message );
    }
    else
    {
        std::abort();
    }
}

