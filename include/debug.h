#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DBG(fmt, ...) \
  do { \
    if (DEBUG) { \
      fprintf(stderr, fmt,  __VA_ARGS__); \
      fprintf(stderr, "\n"); \
    } \
  } while (0)

#endif
