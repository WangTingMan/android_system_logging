
#include <log/porting_log.h>

#include <cutils/properties.h>

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

#define LOGLINE_SIZE 1024
#define BOOLEAN_TRUE 0xFF
#define BOOLEAN_FALSE 0xFE

#define strcasecmp _stricmp

std::mutex s_log_file_mutex;
std::fstream s_log_file_stream;
std::string s_file_name;

template<typename T, int N>
int array_size(T(&arr)[N])
{
    return N;
}

struct cache {
  const void* pinfo;
  uint32_t serial;
};

struct cache_char {
  struct cache cache;
  unsigned char c;
};

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

void ___default_logger(
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

    std::lock_guard<std::mutex> lcker( s_log_file_mutex );
    if( !s_log_file_stream.is_open() )
    {
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

static std::atomic_int32_t minimum_log_priority = ANDROID_LOG_DEFAULT;
static porting_log_callback_type s_cb = nullptr;
static std::mutex lock_loggable;

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
    else
    {
      ___default_logger(a_severity, a_tag, a_fileName, a_funcName, a_lineNr,
                        pBuffer);
    }

    return 0;
}

void __set_porting_log_callback( porting_log_callback_type a_cb )
{
    s_cb = a_cb;
}

void __set_default_log_file_name( const char* a_file_name, int a_auto_change_name )
{
    std::lock_guard<std::mutex> lcker( s_log_file_mutex );
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
    else
    {
      ___default_logger(ANDROID_LOG_ERROR, "", a_fileName, a_funcName, a_lineNr,
                        str.c_str());
    }

    return 0;
}

int __android_log_error_write( int tag, const char* subTag, int32_t uid,
    const char* data, uint32_t dataLen, const char* file, uint32_t line )
{
    if( s_cb )
    {
        s_cb( ANDROID_LOG_ERROR, subTag, file, nullptr, line, data );
    }
    else
    {
      ___default_logger(ANDROID_LOG_ERROR, subTag, file, nullptr, line, data);
    }
    return 0;
}

int __assert_message( const char* file, int line, const char* failed_expression )
{
    if( s_cb )
    {
        s_cb( ANDROID_LOG_ERROR, "", file, nullptr, line, failed_expression );
    }
    else
    {
      ___default_logger(ANDROID_LOG_ERROR, "", file, nullptr, line,
                        failed_expression);
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
    else
    {
      ___default_logger(pri, "", file, nullptr, line, pBuffer);
    }
    return 0;
}

int32_t __android_log_set_minimum_priority(int32_t priority) {
  return minimum_log_priority.exchange(priority, std::memory_order_relaxed);
}

int32_t __android_log_get_minimum_priority() {
  return minimum_log_priority;
}

int __android_log_is_loggable(int prio, const char* tag, int default_prio)
{
    return 0;
}

// It's possible for logging to happen during static initialization before our globals are
// initialized, so we place this std::string in a function such that it is initialized on the first
// call. We use a pointer to avoid exit time destructors.
std::string& GetDefaultTag() {
  static std::string* default_tag = new std::string("default_tag");
  return *default_tag;
}

void __android_log_set_default_tag(const char* tag) {
  GetDefaultTag().assign(tag);
}

static void refresh_cache(struct cache_char* cache, const char* key) {
  char buf[PROP_VALUE_MAX];
  property_get(key,buf,"default");
  switch (buf[0]) {
    case 't':
    case 'T':
      cache->c = strcasecmp(buf + 1, "rue") ? buf[0] : BOOLEAN_TRUE;
      break;
    case 'f':
    case 'F':
      cache->c = strcasecmp(buf + 1, "alse") ? buf[0] : BOOLEAN_FALSE;
      break;
    default:
      cache->c = buf[0];
  }
}

static int __android_log_level(const char* tag, size_t tag_len) {
  if (tag == nullptr || tag_len == 0) {
    auto& tag_string = GetDefaultTag();
    tag = tag_string.c_str();
    tag_len = tag_string.size();
  }

  /*
   * Single layer cache of four properties. Priorities are:
   *    log.tag.<tag>
   *    persist.log.tag.<tag>
   *    log.tag
   *    persist.log.tag
   * Where the missing tag matches all tags and becomes the
   * system global default. We do not support ro.log.tag* .
   */
  static std::string* last_tag = new std::string;
  static uint32_t global_serial;
  uint32_t current_global_serial = 0;
  static cache_char tag_cache[2];
  static cache_char global_cache[2];

  static const char* log_namespace = "persist.log.tag.";
  std::string_view str_view(log_namespace);
  std::vector<char> buffer;
  buffer.resize(str_view.size() + tag_len + 1); 
  char* key = buffer.data();
  strcpy(key, log_namespace);

  bool locked = true;

  std::lock_guard<std::mutex> locker(lock_loggable);

  bool change_detected, global_change_detected;
  global_change_detected = change_detected = !locked;

  char c = 0;
  if (locked) {
    // Check all known serial numbers for changes.
    if (current_global_serial != global_serial) {
      global_change_detected = change_detected = true;
    }
  }

  if (tag_len != 0) {
    bool local_change_detected = change_detected;
    if (locked) {
      // compare() rather than == because tag isn't guaranteed 0-terminated.
      if (last_tag->compare(0, last_tag->size(), tag, tag_len) != 0) {
        // Invalidate log.tag.<tag> cache.
        for (size_t i = 0; i < array_size(tag_cache); ++i) {
          tag_cache[i].cache.pinfo = NULL;
          tag_cache[i].c = '\0';
        }
        last_tag->assign(tag, tag_len);
        local_change_detected = true;
      }
    }

    for (size_t i = 0; i < array_size(tag_cache); ++i) {
      cache_char* cache = &tag_cache[i];
      cache_char temp_cache;

      if (!locked) {
        temp_cache.cache.pinfo = NULL;
        temp_cache.c = '\0';
        cache = &temp_cache;
      }
      if (local_change_detected) {
        refresh_cache(cache, i == 0 ? key : key + strlen("persist."));
      }

      if (cache->c) {
        c = cache->c;
        break;
      }
    }
  }

  switch (toupper(c)) { /* if invalid, resort to global */
    case 'V':
    case 'D':
    case 'I':
    case 'W':
    case 'E':
    case 'F': /* Not officially supported */
    case 'A':
    case 'S':
    case BOOLEAN_FALSE: /* Not officially supported */
      break;
    default:
      /* clear '.' after log.tag */
      key[strlen(log_namespace) - 1] = '\0';

      for (size_t i = 0; i < array_size(global_cache); ++i) {
        cache_char* cache = &global_cache[i];
        cache_char temp_cache;

        if (!locked) {
          temp_cache = *cache;
          if (temp_cache.cache.pinfo != cache->cache.pinfo) {  // check atomic
            temp_cache.cache.pinfo = NULL;
            temp_cache.c = '\0';
          }
          cache = &temp_cache;
        }
        if (global_change_detected) {
          refresh_cache(cache, i == 0 ? key : key + strlen("persist."));
        }

        if (cache->c) {
          c = cache->c;
          break;
        }
      }
      break;
  }

  if (locked) {
    global_serial = current_global_serial;
  }

  switch (toupper(c)) {
    /* clang-format off */
    case 'V': return ANDROID_LOG_VERBOSE;
    case 'D': return ANDROID_LOG_DEBUG;
    case 'I': return ANDROID_LOG_INFO;
    case 'W': return ANDROID_LOG_WARN;
    case 'E': return ANDROID_LOG_ERROR;
    case 'F': /* FALLTHRU */ /* Not officially supported */
    case 'A': return ANDROID_LOG_FATAL;
    case BOOLEAN_FALSE: /* FALLTHRU */ /* Not Officially supported */
    case 'S': return ANDROID_LOG_SILENT;
      /* clang-format on */
  }
  return -1;
}

int __android_log_is_loggable_len(int prio, const char* tag, size_t len, int default_prio) {
  int minimum_log_priority = __android_log_get_minimum_priority();
  int property_log_level = __android_log_level(tag, len);

  if (property_log_level >= 0 && minimum_log_priority != ANDROID_LOG_DEFAULT) {
    return prio >= std::min(property_log_level, minimum_log_priority);
  } else if (property_log_level >= 0) {
    return prio >= property_log_level;
  } else if (minimum_log_priority != ANDROID_LOG_DEFAULT) {
    return prio >= minimum_log_priority;
  } else {
    return prio >= default_prio;
  }
}
