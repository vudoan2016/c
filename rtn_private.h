/*
 *  rtn_private.h
 *
 *  Enke Chen, May 2004
 *
 *  Copyright (c) 2004 Redback Networks, Inc..
 *  Copyright (c) 2014 Ericsson AB.
 *  All rights reserved
 */

#ifndef __RTN_PRIVATE_H__
#define __RTN_PRIVATE_H__


#ifndef     TRUE
#define     TRUE           (1 == 1)
#endif

#ifndef     FALSE
#define     FALSE          (1 == 0)
#endif

#ifndef MIN
#define     MIN(a, b)     (((a) > (b)) ? (b) : (a))
#endif

/*
 * XXX Assumes NBBY is 8.  If it isn't we're in trouble anyway.
 */
#define	RNBBY	     8
#define	RNSHIFT	     3
#define	RNBIT(x)     (0x80 >> ((x) & (RNBBY-1)))
#define	RNBYTE(x)    ((x) >> RNSHIFT)

#define RNODE_CHUNK_ELEM   1000

/* Return the bit position of the msb */
static const u_char first_bit_set[256] = {
    /* 0 - 15 */
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    /* 16 - 31 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    /* 32 - 63 */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    /* 64 - 127 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 128 - 255 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


/*
 * Subtree info used during subtree walk.
 */
typedef struct _stree_info_t
{
    rt_node_t    **st_root;
    int          *exact_exist;
    char         *st_addr;
    u_int16_t    st_bitlen;
    u_int16_t    pad;
} stree_info_t;

#endif /* __RTN_PRIVATE__ */
