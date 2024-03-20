
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
#include <filesystem>

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
        ::base::FilePath path( module_name );
        ::base::FilePath dir = path.DirName();

        path = path.RemoveExtension();
        auto base_name = path.BaseName();

        std::string file_name;
        if( a_auto_change_name )
        {
            file_name = __rotate_file( dir.StdStringValue(),
                                       base_name.StdStringValue(),
                                       "log", 5 );
        }
        else
        {
            file_name = dir.StdStringValue();
            file_name.append( "/" ).append( base_name.StdStringValue() )
                .append( ".log" );
        }
        s_file_name = file_name;
    }
}

std::string __rotate_file
    (
    std::string a_dir,
    std::string a_file_name,
    std::string a_ext_name,
    int a_max_number
    )
{
    if( a_dir.back() != '/' &&
        a_dir.back() != '\\' )
    {
        a_dir.push_back( '/' );
    }

    std::filesystem::path path( a_dir );
    std::error_code err_code;
    if( !std::filesystem::exists( path ) )
    {
        bool result = std::filesystem::create_directory( path, err_code );
        if( !result )
        {
            a_dir.append( a_file_name ).append( "." ).append( a_ext_name );
            return a_dir;
        }
    }

    base::Time::Exploded exploded;
    base::Time now_ = base::Time::NowFromSystemTime();
    now_.LocalExplode( &exploded );
    char buffer[256] = { 0 };
    snprintf( buffer, 256,
              "_%04d%02d%02d_%02d%02d%02d", exploded.year, exploded.month,
              exploded.day_of_month, exploded.hour, exploded.minute,
              exploded.second );
    std::string new_file_name = a_file_name;
    new_file_name.append( buffer ).append( "." ).append( a_ext_name );
    std::string new_file_path( a_dir );
    new_file_path = new_file_path.append( new_file_name );

    std::filesystem::recursive_directory_iterator dir( path ), end;
    std::set<std::filesystem::path> files_found;
    for( ; dir != end; ++dir )
    {
        std::filesystem::path path_cur = dir->path();
        if( !std::filesystem::is_regular_file( path_cur ) )
        {
            continue;
        }

        auto ext = path_cur.extension().generic_string();
        if( !ext.ends_with( a_ext_name ) )
        {
            continue;
        }

        if( ext.size() > a_ext_name.size() + 1 )
        {
            continue;
        }

        auto file_name = path_cur.filename().generic_string();
        if( !file_name.starts_with( a_file_name ) )
        {
            continue;
        }

        files_found.emplace( path_cur );
    }

    if( static_cast<int>( files_found.size() ) < a_max_number )
    {
        return new_file_path;
    }

    int delete_count = files_found.size() - a_max_number;
    int i = 0;
    for( auto it = files_found.begin();
         ( it != files_found.end() ) && ( i < delete_count );
         ++it, ++i )
    {
        std::filesystem::remove( *it );
    }

    return new_file_path;
}

