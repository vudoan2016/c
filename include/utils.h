#ifndef __MISC_H
#define __MISC_H

#define BA_MAX 0x7ffffff

static inline void ba_init(unsigned int *ba)
{
  memset(ba, 0, sizeof(unsigned int)*BA_MAX);
}

static inline bool ba_search(unsigned int *ba, int x)
{
  return (ba[x>>5] & (1 << x%32));
}

static inline void ba_insert(unsigned int *ba, int x)
{
  ba[x>>5] |= 1 << x%32;
}

#endif
