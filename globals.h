#ifndef __GLOBALS_H
#define __GLOBALS_H

#define DEBUG 1

#define DBG(fmt, ...)				\
  do { \
    if (DEBUG) { \
      fprintf(stderr, fmt, __VA_ARGS__); \
    } \
  } while (0)

#endif
