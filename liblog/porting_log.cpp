
#include <android/log.h>

#include <atomic>
#include <stdarg.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <mutex>
#include <string_view>
#include <vector>
#include <fstream>

#include <base/debug/stack_trace.h>
#include <base/files/file_util.h>
#include <base/strings/string_util.h>
#include <base/threading/platform_thread.h>
#include <base/threading/thread.h>
#include <base/time/time.h>
#include <base/logging.h>

std::recursive_mutex s_log_file_mutex;
std::fstream s_log_file_stream;
std::string s_file_name;

const char *___extract_file_name_from_path(const char *path) {
  const char *file = path;
  const char *split = path;
  while (file && *(file++) != '\0') {
    if ('/' == *file || '\\' == *file) {
      split = file + 1;
    }
  }
  if (!split) {
    split = "Unkown";
  }
  return split;
}

void ___default_logger
        (
        android_LogPriority a_severity,
        const char* a_tag,
        const char* a_fileName,
        const char* a_funcName,
        unsigned int a_lineNr,
        const char* a_pStr
        )
{
    const char* file = ___extract_file_name_from_path(a_fileName);
    std::stringstream ss;

    base::Time::Exploded exploded_time;
    base::Time current_time = base::Time::NowFromSystemTime();
    current_time.LocalExplode( &exploded_time );

    char buffer[256] = { 0 };
    snprintf( buffer, 256,
              "[%02d-%02d %02d:%02d:%02d.%03d] ", exploded_time.month,
              exploded_time.day_of_month, exploded_time.hour, exploded_time.minute,
              exploded_time.second, exploded_time.millisecond );
    ss << buffer;

    switch( a_severity )
    {
    case ANDROID_LOG_UNKNOWN:
        ss << "[V] ";
        break;
    case ANDROID_LOG_DEFAULT:
        ss << "[V] ";
        break;
    case ANDROID_LOG_VERBOSE:
        ss << "[V] ";
        break;
    case ANDROID_LOG_DEBUG:
        ss << "[D] ";
        break;
    case ANDROID_LOG_INFO:
        ss << "[I] ";
        break;
    case ANDROID_LOG_WARN:
        ss << "[W] ";
        break;
    case ANDROID_LOG_ERROR:
        ss << "[E] ";
        break;
    case ANDROID_LOG_FATAL:
        ss << "[F] ";
        break;
    case ANDROID_LOG_SILENT:
        ss << "[V] ";
        break;
    default:
        break;
    }

    ss << "[" << base::PlatformThread::CurrentId() << "] ";

    ss << "[" << file << ':' << a_lineNr << "] ";
    if (a_pStr)
    {
        ss << a_pStr;
    }

    std::string log_str = ss.str();
    if (log_str.back() != '\n')
    {
        log_str.push_back('\n');
    }

    std::lock_guard<std::recursive_mutex> lcker( s_log_file_mutex );
    if( !s_log_file_stream.is_open() )
    {
        if( s_file_name.empty() )
        {
            __set_default_log_file_name( nullptr, false );
        }

        if( !s_file_name.empty() )
        {
            s_log_file_stream.open( s_file_name, std::ios::out | std::ios::trunc );
        }
    }

    if( s_log_file_stream.is_open() )
    {
        s_log_file_stream << log_str << std::flush;
    }
    else
    {
        std::cout << log_str;
    }

    if (a_severity == ANDROID_LOG_FATAL)
    {
        std::abort();
    }
}

extern "C" void __android_log_logd_logger_default( const struct __android_log_message* log_message )
{
    ___default_logger( android_LogPriority( log_message->priority ),
                       log_message->tag, log_message->file, nullptr, log_message->line,
                       log_message->message );
}

extern "C" void __set_default_log_file_name(const char* a_file_name, int a_auto_change_name)
{
    std::lock_guard<std::recursive_mutex> lcker( s_log_file_mutex );
    if( a_file_name )
    {
        s_file_name = a_file_name;
    }
    else
    {
        wchar_t module_name[MAX_PATH];
        GetModuleFileName( nullptr, module_name, MAX_PATH );
        std::wstring log_name = module_name;
        std::wstring::size_type last_backslash = log_name.rfind( '.', log_name.size() );
        if( last_backslash != std::wstring::npos )
            log_name.erase( last_backslash + 1 );
        ::base::FilePath path( log_name );
        std::string file_name = path.StdStringValue();
        if( a_auto_change_name )
        {
            int pid = _getpid();
            file_name.append( std::to_string( pid ) ).append( ".log" );
        }
        else
        {
            file_name.append( "log" );
        }
        s_file_name = file_name;
    }
}

