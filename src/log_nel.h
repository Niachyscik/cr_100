#ifndef LOG_NEL_H_INCLUDED
#define LOG_NEL_H_INCLUDED

#include <stdio.h>
#include <string.h>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
#define _CONCAT(x,y) x##y
#define CONCAT(x,y) _CONCAT(x,y)

#if defined DEBUG_TARGET && defined LOG_MODULE
#define LOG_INFO(fmt, ...)  printf("<info> " STRINGIFY(LOG_MODULE)": " fmt"\n",##__VA_ARGS__)
#define LOG_ALARM(fmt, ...)  printf("<alarm> " STRINGIFY(LOG_MODULE)": " fmt"\n",##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) (void)0
#define LOG_ALARM(fmt, ...) (void)0
#endif /* (DEBUG_TARGET) */

#endif /* LOG_NEL_H_INCLUDED */

