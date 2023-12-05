#pragma once

#if defined(LIBLOG_IMPLEMENTATION)
#define LIBLOG_EXPORT __declspec(dllexport)
#else
#define LIBLOG_EXPORT __declspec(dllimport)
#endif  // defined(LIBLOG_IMPLEMENTATION)


