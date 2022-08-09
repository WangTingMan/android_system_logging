
#include <log/porting_log.h>

#include <stdarg.h>
#include <stdio.h>
#include <iostream>
#include <string>

#define LOGLINE_SIZE 1024

static porting_log_callback_type s_cb = nullptr;

int __log_format(
    android_LogPriority a_severity,
    const char* a_tag,
    const char* a_fileName,
    const char* a_funcName,
    unsigned int a_lineNr,
    const char* a_pStr,
    ... )
{
    char pBuffer[LOGLINE_SIZE];
    va_list vaList;
    va_start( vaList, a_pStr );
    ( void )vsnprintf( pBuffer, LOGLINE_SIZE - 1, a_pStr, vaList );
    va_end( vaList );

    if( s_cb )
    {
        s_cb( a_severity, a_tag, a_fileName, a_funcName, a_lineNr, pBuffer );
    }

    return 0;
}

void __set_porting_log_callback( porting_log_callback_type a_cb )
{
    s_cb = a_cb;
}

int __log_error_stamp(
    const char* a_fileName,
    const char* a_funcName,
    unsigned int a_lineNr,
    uint32_t a_number,
    const char* a_pStr
    )
{
    std::string str;
    str.append( "write error log. number: " ).append( std::to_string( a_number ) )
        .append( ", message: " ).append( a_pStr );

    if( s_cb )
    {
        s_cb( ANDROID_LOG_ERROR, "", a_fileName, a_funcName, a_lineNr, str.c_str() );
    }

    return 0;
}

int __android_log_error_write( int tag, const char* subTag, int32_t uid,
    const char* data, uint32_t dataLen )
{
    if( s_cb )
    {
        s_cb( ANDROID_LOG_ERROR, subTag, nullptr, nullptr, 0, data );
    }
    return 0;
}

int __assert_message( const char* file, int line, const char* failed_expression )
{
    if( s_cb )
    {
        s_cb( ANDROID_LOG_ERROR, "", file, nullptr, line, failed_expression );
    }
    return 0;
}

int ___syslog_message( int level, const char* file, int line, const char* a_pStr, ... )
{
    char pBuffer[LOGLINE_SIZE];
    va_list vaList;
    va_start( vaList, a_pStr );
    ( void )vsnprintf( pBuffer, LOGLINE_SIZE - 1, a_pStr, vaList );
    va_end( vaList );

    android_LogPriority pri = static_cast< android_LogPriority>( level );

    if( s_cb )
    {
        s_cb( pri, "", file, nullptr, line, pBuffer );
    }
    return 0;
}

