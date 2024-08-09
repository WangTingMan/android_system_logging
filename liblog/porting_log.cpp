
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
#include <shared_mutex>
#include <vector>
#include <fstream>
#include <filesystem>
#include <unordered_map>

#include <base/debug/stack_trace.h>
#include <base/files/file_util.h>
#include <base/strings/string_util.h>
#include <base/threading/platform_thread.h>
#include <base/threading/thread.h>
#include <base/time/time.h>
#include <base/logging.h>
#include <base/strings/sys_string_conversions.h>

std::recursive_mutex s_log_file_mutex;
std::fstream s_log_file_stream;
std::string s_file_name;
std::string s_original_name;
int s_max_file_count = 5;
int s_max_line_number = 1000000;
int s_current_line_number = 0;

class file_name_cacher
{

public:

    static file_name_cacher& get_instance();

    const char* extract_file_name_from_path( const char* path );

private:

    const char* find_file_name_from_path( const char* path );

    std::shared_mutex m_mutex;
    std::unordered_map<const char*, const char*> m_cached_file_names;
};

file_name_cacher& file_name_cacher::get_instance()
{
    static file_name_cacher instance;
    return instance;
}

const char* file_name_cacher::extract_file_name_from_path( const char* path )
{
    std::shared_lock<std::shared_mutex> sh_locker( m_mutex );
    auto it = m_cached_file_names.find( path );
    if( it != m_cached_file_names.end() )
    {
        return it->second;
    }

    sh_locker.unlock();

    const char* base_name = find_file_name_from_path( path );

    std::lock_guard<std::shared_mutex> locker( m_mutex );
    m_cached_file_names[path] = base_name;

    return base_name;
}

const char * file_name_cacher::find_file_name_from_path(const char *path) {
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
    const char* file = file_name_cacher::get_instance().extract_file_name_from_path( a_fileName );
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
        s_current_line_number++;
        s_log_file_stream << log_str << std::flush;
        if( s_current_line_number > s_max_line_number )
        {
            s_current_line_number = 0;
            s_log_file_stream.close();
            s_file_name = __rotate_file( s_original_name, s_max_file_count );
            s_log_file_stream.open( s_file_name, std::ios::out | std::ios::trunc );
        }
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
        s_original_name.assign( a_file_name );
        s_file_name = s_original_name;
        if( a_auto_change_name )
        {
            s_file_name = __rotate_file( s_original_name, 5 );
        }
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

extern "C" void __set_file_log_attributes
    (
    int a_max_file_count,
    int a_max_line_number
    )
{
    std::lock_guard<std::recursive_mutex> lcker( s_log_file_mutex );
    s_max_file_count = a_max_file_count > 0 ? a_max_file_count : s_max_file_count;
    s_max_line_number = a_max_line_number > 1000 ? a_max_line_number : s_max_line_number;
}

std::string __rotate_file
    (
    std::string a_path,
    int a_max_number
    )
{
    std::string ret_path;

    ::base::FilePath path( a_path );
    ::base::FilePath dir = path.DirName();
    auto ext = path.FinalExtension();
    std::string ext_name;
    if( ext.empty() )
    {
        ext_name = "log";
    }
    else
    {
        std::string ext_name_ = base::SysWideToNativeMB( ext );
        auto point_pos = ext_name_.find_last_of( '.' );
        if( point_pos != std::string::npos )
        {
            if( ext_name_.size() > 1 )
            {
                ext_name = ext_name_.substr( point_pos + 1 );
            }
            else
            {
                ext_name = "log";
            }
        }
        else
        {
            ext_name = ext_name_;
        }
    }

    path = path.RemoveExtension();
    auto base_name = path.BaseName();

    ret_path = __rotate_file( dir.StdStringValue(), base_name.StdStringValue(), ext_name, a_max_number );

    return ret_path;
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
        std::error_code error;
        std::string error_str;
        bool ret = std::filesystem::remove( *it, error );
        if( !ret )
        {
            error_str = error.message();
        }
    }

    return new_file_path;
}

