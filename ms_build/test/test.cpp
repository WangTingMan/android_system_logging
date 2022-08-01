
#include <iostream>
#include <log\log.h>

#define LOG_TAG "HH"

void porting_log_callback(
    android_LogPriority a_severity,
    const char* a_tag,
    const char* a_fileName,
    const char* a_funcName,
    unsigned int a_lineNr,
    const char* a_pStr
    )
{
    std::string str( a_pStr );
    std::cout << str;
}


int main()
{
    __set_porting_log_callback( &porting_log_callback );

    ALOGV( "jell%d", 123 );
    __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, "0x1234455" );
    android_errorWriteLog( 0x01, "0x01" );
    LOG_ALWAYS_FATAL_IF( 1 < 2, "this ok: %d", 89 );
    ALOG_ASSERT( 1 < 2, "this ok: %d", 89 )

    std::cout << "Hello World!\n";
}

