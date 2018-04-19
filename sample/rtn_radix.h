/**
 *  @name rtn_radix.h, Versioning radix Tree
 *
 *  API for rtn_radix.c,
 *
 *  Enke Chen, October 1999
 *  Acee Lindem, July 1999 (Turbo Walk)
 *
 *     Copyright (c) 2009 RedBack Networks, Inc.
 *     Copyright (c) 2010, 2014-2016 Ericsson AB.
 *
 *     All rights reserved.
 */

#ifndef __RTN_RADIX_H__
#define __RTN_RADIX_H__

#include <sys/types.h>
#include <stdarg.h>
#include "core-data-types/baseTypes.h"

#define     RTWALK_ABORT        -1
#define     RTWALK_CONTINUE      0

/*
 * Flags for a radix node (internal or external).
 *
 * An external node should always have the RNODE_EXTERNAL flag, but it
 * may not have RNODE_INFO due to delayed deletion.
 */
#define  RNODE_INFO        0x01    /* valid info */
#define  RNODE_EXTERNAL    0x02    /* external node */
#define  RNODE_DELETED     0x04    /* marked for deletion */
#define  RNODE_REPLACED    0x08    /* node replaced (not in the tree) */

/*
 * Shared by the internal node and the external node.
 */
#define RNODE_COMMON                                                     \
    struct _rt_node_t   *rnode_left;          /* child when bit clear */ \
    struct _rt_node_t   *rnode_right;         /* child when bit set */   \
    struct _rt_node_t   *rnode_parent;        /* parent */               \
    u_int16_t           rnode_bit;            /* bit number for node */  \
    u_int8_t            rnode_flags;          /* RNODE_ flag here */     \
    u_int8_t            rnode_lock;           /* locks on this node */   \
    u_int32_t           rnode_version;        /* max. subtree version */ \

/*
 * Structure for an internal radix node, which does not have any info
 * associated.
 */
typedef struct _rt_node_t {
    RNODE_COMMON;
} rt_node_t;

/*
 * Structure for an external radix node, which has the info embedded
 * in the structure.
 *
 * Note that rninfo_key is in the network byte order.
 *
 * The radix_info should be part of a structure with more attributes
 * specified by a user, and rninfo_key should point to the actual
 * key field in the structure.
 */
#define RADIX_INFO_HEADER                                            \
  RNODE_COMMON;                                                      \
  void                 *rninfo_key;         /* NETWORK BYTE ORDER */ \
  u_int32_t            version;             /* version for info */   \


typedef struct _rt_info_t {
    RADIX_INFO_HEADER;
} rt_info_t;

/*
 * convert info to node.
 */
#define RADIX_INFO2NODE(ri)    ((rt_node_t *) ri)

/*
 * Function pointer to free up a radix_info entry. We use free()
 * when this function is not supplied.
 */
typedef void (*rt_info_free)(rt_info_t *rinfo);

/**
 * Defs to hold the radix root, function pointers and the radix stats.
 *
 * The function pointer could be NULL in which case free() is used
 * to free up the info part.
 *
 * When RTN_BIT_KEEP_INFO is set, the info part is not freed after it is
 * detached from the tree, and a user needs to explicitly free up the
 * memory. This can be useful, e.g., when the info is part of a larger
 * contiguous memory block. This should be used with extra care !!!
 *
 * When RTN_BIT_MULTI_INFO is set, it means that there are multiple info
 * entries chained off a node, and we can not simply look at the node
 * version to decide if the node can be skipped in rtn_walktree_version().
 * The user function supplied must perform the skipping (a corner case).
 */
#define RTN_BIT_CHUNK_NONE     0x00  /* do not use chunk for node */
#define RTN_BIT_USE_CHUNK      0x01  /* use chunk for node */
#define RTN_BIT_KEEP_INFO      0x02  /* do not free rt_info */
#define RTN_BIT_MULTI_INFO     0x04  /* multiple info entries for a node */
#define RTN_BIT_USE_CHUNK2     0x08  /* use the new chunk for node */

/*
 * re-map some old defs.
 */
#define RNODE_CHUNK_NONE       RTN_BIT_CHUNK_NONE
#define RNODE_CHUNK_SHARED     RTN_BIT_USE_CHUNK

typedef struct _rt_head_t
{
    rt_node_t *root;          /* radix trie root */
    rt_info_free ri_free;     /* function to free user info */
    u_int8_t  flags;          /* RTN_BIT_ flags */
    u_int8_t  pad3[3];        /* for alignment */

    u_int32_t rn_count;       /* count of internal nodes */
    u_int32_t ri_count;       /* count of external nodes */
    u_int32_t rtn_walktree_count;  /* count of calling rtn_walktree() */
    u_int32_t rtn_walktree_version_count;  /* count of calling rtn_walktree() */
} rt_head_t;


/**
 * Walk function to process an info item.
 *
 * @return
 *     RTWALK_ABORT    - run into error, or need to abort.
 *     RTWALK_CONTINUE - no error and continue to next entry.
 */
typedef int (*rtn_walk_func)(rt_info_t *rinfo, va_list ap);


/**
 * Walk function to process (e.g., dump) a node (NOT info).
 *
 */
typedef void (*rnode_walk_func)(rt_node_t *rn);


/**
 * @name API for radix.c
 */

/**
 * Function to initialize the root of a tree. This should be called
 * and the root be remembered by an application before it can do
 * anything with the tree.
 *
 * @param rt_head  Structure for root, free and stats. Can not be NULL.
 * @param chunk_flags  Should be one of RNODE_CHUNK_ flags.
 * @param ri_free  The function pointer to free the user info. It could
 *                 be NULL in which case free() is used.
 * @return
 *     Return the root, could be NULL.
 */
extern rt_node_t *rtn_root_init(rt_head_t *rt_head, u_int8_t flags,
                                rt_info_free ri_free);

/*
 * Function to free the root when a radix tree is deleted.
 *
 * @param rt_head  structure that holds the root and stats. Can not be NULL.
 */
extern void rtn_root_free(rt_head_t *rt_head);


/**
 * Insert info to a tree. One should use rtn_search() to check if
 * the node exists first.
 *
 * @param rt_head     head structure. Must not be NULL.
 * @param rinfo       a radix_info entry that we try to insert
 * @param bitlen      bit length for the radix_info
 *
 * @return
 *     TRUE: succeed; FALSE: fail.
 */
extern int8_t rtn_add(rt_head_t *rt_head, rt_info_t *rinfo, u_int16_t bitlen);


/**
 * Delete an info entry.
 *
 * @param rinfo      the radix info to be delted
 * @param rt_head    ptr to the head structure, can not be NULL.
 *
 * @return
 *     void
 */
extern void rtn_delete(rt_info_t *rinfo, rt_head_t *rt_head);


/**
 * Increment the table version, then set the version number for the entry,
 * and update the subtree max version of all its parents.
 * Return TRUE if there is a version wrap.
 *
 * @param rinfo           an info entry
 * @param table_version   table/tree version
 *
 * @return
 *      TRUE:    if there is a wrap of the table version
 *      FALSE:   there is no version wrap
 */
extern int8_t rtn_bump_version(rt_info_t *rinfo, u_int32_t *table_version);
extern int8_t rtn_bump_version_max_limit(rt_info_t *rinfo, u_int32_t *table_version, u_int32_t max);


/**
 * Do exact match of a prefix.
 *
 * @param rt_head the ptr to the head structure
 * @param addr    address in the network byte order
 * @param bitlen  bit length of the address
 *
 * @return
 *      radix info found, could be NULL.
 */
extern rt_info_t *rtn_search(rt_head_t *rt_head, char *addr, u_int16_t bitlen);


/**
 * Perform best (i.e., longest) match for an address.
 *
 * @param rt_head     the ptr to the head structure.
 * @param addr        address in the network byte order
 * @param maxbitlen   the max bit length allowed, that is, a radix info
 *                    returned must not have bit length larger than this.
 *
 * @return
 *      radix info found, could be NULL.
 */
extern rt_info_t *rtn_lookup(rt_head_t *rt_head, char *addr, u_int16_t maxbitlen);
extern rt_node_t *rtn_lookup_node (rt_head_t *rt_head, char *addr,
                                   u_int16_t bitlen);

extern rt_node_t *
rtn_walk_ascend (rt_node_t *st_root, rt_node_t *rn);

extern rt_node_t *
rtn_walk_next_node (rt_node_t *st_root, rt_node_t *rn);


/**
 * Find the shortest match that is covered by the specified prefix. In other words,
 * this function will return either the first or "root" node for a range specified
 * by a prefix.
 *
 * @param rt_head     ptr to the head structurem cannot be null.
 * @param addr        address in the network byte order
 * @param maxbitlen   The prefix length for the range comparison. Any returned
 *                    route info node must have a prefix length that >= to
 *                    this value in order to be covered by the range.
 *
 * @return
 *      radix info found, could be NULL.
 */
rt_info_t *rtn_lookup_range(rt_head_t *rt_head, char *addr, u_int16_t maxbitlen);



/*
 * rtn_lookup_prefix_overlap
 *
 * Find any prefix overlap between the given addr and nodes on the tree.
 * 1. first search external nodes on the tree with
 * rnode_bit smaller than maxbitlen and check if prefix of rnode_bit
 * length matches that of the given address. if yes, return the node. if no,
 * 2. search external nodes on the tree with rnode_bit bigger than
 * maxbitlen and check if  prefxi of maxbitlen length matches that of the
 * given address. if yes, return the node. otherwise return NULL.
 */
rt_info_t *
rtn_lookup_prefix_overlap (rt_head_t *rt_head, char *addr, u_int16_t
                           maxbitlen);


/**
 * Walk all entries, and yield after the specified number of entries
 * have been processed if blocking is TRUE.
 *
 * @param start_node  node to start the walk, usually root.
 * @param f           walk function to process an radix info.
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param limit_ms    duration (in ms) of processing before yield when
 *                    blocking is TRUE.
 * @param blocking    TRUE: yield; FALSE: not yield.
 *
 * @return
 *      0: walktree successful.
 */
extern int8_t rtn_walktree(rt_node_t *start_node, rtn_walk_func f,
                           rt_head_t *rt_head, u_int32_t limit_ms,
                           int8_t blocking, ...);


/**
 * Version walk. Process entries with version number greater than
 * min_version, and smaller or equal to max_version.
 *
 * @param start_node  node to start the walk, usually root.
 * @param f           walk function to process an radix info.
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param min_version version of a radix info must be larger than this.
 * @param max_version versoin of a radix info must be small than
 *                    (or equal to) this value.
 * @param limit_ms    duration (in ms) of processing before yield when
 *                    blocking is TRUE.
 * @param blocking    TRUE: yield; FALSE: not yield.
 *
 * @return
 *      0: walktree successful.
 */
extern int8_t rtn_walktree_version(rt_node_t *start_node, rtn_walk_func f,
                                   rt_head_t *rt_head, u_int32_t min_version,
                                   u_int32_t max_version, u_int32_t limit_ms,
                                   int8_t blocking, ...);

/*
 * rtn_walktree_keybits
 *
 * This function walks entries with keybits between (min_keybits,
 * max_keybits). When the specified max_keybits is smaller than the
 * max bits of the tree, this function is more efficient than walking
 * the entire tree. The arguments of this function are similar to those
 * in rtn_walktree_version().
 */
extern int8_t rtn_walktree_keybits(rt_node_t *start_node, rtn_walk_func f,
                                   rt_head_t *rt_head, u_int32_t min_keybits,
                                   u_int32_t max_keybits, u_int32_t limit_ms,
                                   int8_t blocking, ...);

/**
 * Get the next (lexically) info entry that is below the parent_node.
 * The return result may include the exact match if it is TRUE.
 *
 * @param rt_head      ptr to the head structure, can not be NULL.
 * @param addr         address in network byte order.
 * @param bitlen       bit length.
 * @param exact_match  TRUE: return exact match if it exists.
 *
 * @return
 *       an radix info.
 */
extern rt_info_t *rtn_key_getnext(rt_head_t *rt_head, char *addr,
                                  u_int16_t bitlen, int exact_match);

/**
 * Walk limited number of entries, and then return.
 * The next_item is the item at which we stop processing (i.e., not processed).
 * The cur_item is the item after which we stop processing (i.e already processed).
 *
 * @param start_node  node to start the walk. It should either be the root
 *                    or the node at which we stop processing previously.
 * @param f           walk function to process an radix info.
 * @next_item         last info entry that we stop processing in this walk.
 *                    NULL means that we have walked the whole tree.
 * @cur_item          last info entry that we processed(already) in this walk.
 *                    NULL means that we have walked the whole tree.
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param limit       number of entries to process before yield when
 *                    blocking is TRUE.
 *
 * @return
 *      0: walk successful, no error.
 *
 */
extern int8_t rtn_walktree_limited_curnext(rt_node_t *start_node, rtn_walk_func f,
                                   rt_info_t **next_item, rt_info_t **cur_item,
                                   rt_head_t *rt_head, u_int32_t limit, ...);

/**
 * Walk limited number of entries, and then return.
 * The last_item is the item at which we stop processing (i.e., not processed).
 *
 * @param start_node  node to start the walk. It should either be the root
 *                    or the node at which we stop processing previously.
 * @param f           walk function to process an radix info.
 * @last_item         last info entry that we stop processing in this walk.
 *                    NULL means that we have walked the whole tree.
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param limit       number of entries to process before the walk returns
 *
 * @return
 *      0: walk successful, no error.
 *
 */
extern int8_t rtn_walktree_limited(rt_node_t *start_node, rtn_walk_func f,
                                   rt_info_t **last_item, rt_head_t *rt_head,
                                   u_int32_t limit, ...);


/**
 * Version walk of a limited number of entries, and then return.
 *
 * @param start_node  node to start the walk, usually root.
 * @param f           walk function to process an radix info.
 * @last_item         last info entry that we stop processing in this walk.
 *                    NULL means that we have walked the whole tree.
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param min_version version of a radix info must be larger than this.
 * @param max_version versoin of a radix info must be small than
 *                    (or equal to) this value.
 * @param limit       number of entries to process before yield when
 *                    blocking is TRUE.
 *
 * @return
 *      0: walk successful.
 */
extern int8_t
rtn_walktree_version_limited(rt_node_t *start_node, rtn_walk_func f,
                             rt_info_t **last_item, rt_head_t *rt_head,
                             u_int32_t min_version,
                             u_int32_t max_version, u_int32_t limit, ...);


/**
 * Walk a subtree (ie. all entries below a given prefix), and yield after
 * the specified number of entries have been processed if blocking is TRUE.
 *
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param st_addr     subtree address in the network byte order.
 * @param st_bitlen   subtree bit length specified.
 * @param fi          walk function to process an radix info.
 * @param limit_ms    duration of processing before yield when
 *                    blocking is TRUE.
 * @param blocking    TRUE: yield; FALSE: not yield.
 *
 * @return
 *      0: walktree successful.
 */
extern int8_t rtn_walk_subtree(rt_head_t *rt_head, char *st_addr,
                               u_int16_t st_bitlen, rtn_walk_func fi,
                               u_int32_t limit_ms, int8_t blocking, ...);


/**
 * Walk a limited number of entries of a subtree, and then return.
 * The last_item is the item at which we stop processing (i.e., not processed).
 * This is useful in displaying a subtree.
 *
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param st_addr     address (network byte order) of the subtree.
 * @param st_bitlen   the bit length of the subtree.
 * @param prev_addr   the address at which the previous walk stopped.
 * @param prev_bitlen the bitlen at which the previous walk stopped.
 *                    Use the value of 0 for the first round.
 * @param fi          walk function to process an radix info.
 * @last_item         last info entry that we stop processing in this walk.
 *                    NULL means that we have walked the whole tree.
 * @exact             this is passed directly to rtn_key_getnext()
 * @param limit       number of entries to process before yield when
 *                    blocking is TRUE.
 *
 * @return
 *      0: walk successful, no error.
 *
 */
extern int8_t rtn_walk_subtree_limited(rt_head_t *rt_head, char *st_addr,
                                       u_int16_t st_bitlen, char *prev_addr,
                                       u_int16_t prev_bitlen, rtn_walk_func fi,
                                       rt_info_t **last_item,
                                       bool exact, u_int32_t limit, ...);

/*
 * Perform a versioned subtree walk.
 *
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param st_addr     subtree root address (network byte order).
 * @param st_bitlen   subtree root bit length.
 * @param last_addr   start walk from this address/bitlen (could be NULL).
 * @param last_bitlen ...
 * @param fi          walk function to process an radix info.
 * @param min_version version of a radix info must be larger than this.
 * @param max_version versoin of a radix info must be small than
 *                    (or equal to) this value.
 * @param limit       number of entries to process before yield when
 *                    blocking is TRUE.
 */
int
rtn_walk_subtree_version(rt_head_t *rt_head, u_char *st_addr,
                         u_int16_t st_bitlen, u_char *last_addr,
                         u_int16_t last_bitlen, rtn_walk_func fi,
                         u_int32_t min_version, u_int32_t max_version,
                         u_int32_t limit_ms, int8_t blocking, ...);

/**
 * Walk the entire tree and dump its content, including the hex pointers.
 * Two functions fi and fn must be supplied to display the info and node.
 *
 * @param start_node  node to start the walk, usually root.
 * @param fi          walk function to display a radix info.
 * @param fn          walk function to display a radix node.
 * @param rt_head     ptr to the head structure, can not be NULL.
 * @param limit_ms    duration of processing before yield when
 *                    blocking is TRUE.
 * @param blocking    TRUE: yield; FALSE: not yield.
 *
 * @return
 *      0: walktree successful.
 */
int8_t
rtn_dumptree (rt_node_t *start_node, rtn_walk_func fi, rnode_walk_func fn,
              rt_head_t *rt_head, u_int32_t limit_ms, int8_t blocking, ...);

/*
 * Attach the subtree with given prefix string and len to the
 * main tree. Return error if subtree is already part of main tree.
 *
 * @param rt_head     Main tree head structure. Must not be NULL.
 * @param st_root     root node of subtree to be attached.
 * @param addr        subtree prefix address in the network byte order
 * @param bitlen      subtree prefix bit length
 *
 * @return
 *     TRUE: succeed; FALSE: fail.
 */
int8_t
rtn_attach_subtree(rt_head_t *rt_head, rt_node_t *st_root,
                   char *addr, u_int32_t bitlen);

/*
 * Find and detach the subtree with given prefix string and len
 * from the main tree.
 *
 * @param rt_head     Main tree head structure. Must not be NULL.
 * @param prefix      subtree prefix address in the network byte order
 * @param bitlen      subtree prefix bit length
 *
 * @return            subtree root node if found else NULL.
 *
 */
rt_node_t *
rtn_detach_subtree(rt_head_t *rt_head, char *prefix, u_int32_t bitlen);


/*
 * Return parent info node of passed node.
 *
 * @param rnode      a tree node
 *
 * @return           parent info node
 *
 */
static inline rt_info_t *
rtn_get_parent_info_node (rt_node_t *rnode)
{
    rt_node_t *rn_last;
    for (rn_last = rnode->rnode_parent; rn_last;
            rn_last = rn_last->rnode_parent) {
        if (rn_last->rnode_flags & RNODE_INFO) {
            break;
        }
    }
    return ((rt_info_t *)rn_last);
}

/**
 * Get info node at address.
 *
 * @param rt_head the ptr to the head structure
 * @param addr    address in the network byte order
 * @param bitlen  bit length of the address. retuned info node should have
 *                bit length equal or greater than this
 *
 * @return
 *      radix info found, could be NULL.
 */
extern rt_info_t *
rtn_get_info_node(rt_head_t *rt_head, char *addr, u_int16_t bitlen);


/*-----------------------------------------------------------------------
 *  Turbo Walk Macros
 *-----------------------------------------------------------------------
 *  The turbo walk macros provide a low overhead pre-order traversal of
 *  SmartEdge's radix trie. They can be used for non-premptive full and
 *  partial walks in situations where the full capabilities of the functions
 *  in the radix library are not required (e.g., versioning and quantum
 *  checking) and a lower overhead traversal is desired.
 */

/*
 * Start/Stop walk macros - Node specific processing should be
 * bracketed by these macros. Parameters:
 *
 *  start_p -  rt_node_t pointer to the starting node.
 *  info_p  -  User pointer to rt_info_t pointer.
 *  type    -  Type to cast info_p to at each node in the traversal.
 *  end_p   -  rt_node_t pointer to the ending node. This can be identical
 *             to the start_p for a sub-tree walk or a different node
 *             altogether (for example, the root of the tree).
 */
#define TURBO_RADIX_WALK_START(start_p, info_p, info_type) \
 { \
    rt_node_t           *turbo_node_p; \
    turbo_node_p = (start_p); \
    do { \
        if (turbo_node_p->rnode_flags & RNODE_INFO) { \
            (info_p) = (info_type) turbo_node_p;

#define TURBO_RADIX_WALK_END(end_p) \
        } \
        if (turbo_node_p->rnode_left != NULL) { \
           turbo_node_p = turbo_node_p->rnode_left; \
        } else { \
           if (turbo_node_p->rnode_right != NULL) { \
               turbo_node_p = turbo_node_p->rnode_right; \
           } else { \
               while (turbo_node_p->rnode_parent != NULL) { \
                  if ((turbo_node_p->rnode_parent->rnode_right == turbo_node_p) \
                     || (turbo_node_p->rnode_parent->rnode_right == NULL)) { \
                      turbo_node_p = turbo_node_p->rnode_parent; \
                  } else { \
                      turbo_node_p = turbo_node_p->rnode_parent->rnode_right; \
                      break; \
                  } \
               } \
           } \
        } \
    } while (turbo_node_p != (end_p)); \
 }

#endif  /* __RTN_RADIX_H__ */
