/***
 *   rtn_radix.c
 *
 *   Code for radix trie (including versioning)
 *
 *   Enke Chen
 *
 *   October 1999
 *
 *    Copyright (c) 2009 Redback Networks Inc.
 *    Copyright (c) 2010, 2014-2016 Ericsson AB.
 *    All rights reserved.
 *
 ***
 * Description:
 *
 * This is another implementation of radix code. There is a good
 * description from GateD on "how this works". However, there are
 * several major differences with the GateD approach:
 *
 *   o Use prefixlen instead of mask, thus non-contiguous mask
 *     is not supported.
 *
 *   o Use a uchar pointer as the key instead of sockhead.
 *
 *   o Walk up the tree in doing best match.
 *
 *   o Support of delayed deletion to allow multiple access to
 *     the tree.
 *
 *   o Pre-order tree walk
 *
 * several functions have been adapted from the Gated rt_radix.c.
 *
 ***/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <pthread.h>
#include "platform-os/pthread_np.h"

#include "corelibs/chunk.h"
#include "corelibs/rtn_radix.h"
#include "rtn_private.h"

/*
 * For yield. This should have been passed in tree walk.
 */
static struct timeval rtn_wc_tv = {0, 500000};        /* 1/2 second */

/*
 * chunk for radix node.
 */
static chunk_header_t *rnode_chunk_header = NULL;

/*
 * rtn_node_alloc
 *
 * Allocate an internal node.
 */
static inline rt_node_t *
rtn_node_alloc (rt_head_t *rt_head)
{
    rt_node_t *node;

    node = chunk_alloc(rnode_chunk_header, TRUE);
    if (node) {
        rt_head->rn_count++;
    }

    return (node);
}

/*
 * rtn_node_free
 *
 * Free a node (internal or external).
 */
static inline void
rtn_node_free (rt_head_t *rt_head, rt_node_t *rn)
{
    if (rn->rnode_flags & RNODE_EXTERNAL) {
        if (rt_head->ri_free) {
            (*(rt_head->ri_free))((rt_info_t *)rn);
        } else if (!(rt_head->flags & RTN_BIT_KEEP_INFO)) {
            free(rn);
        }
        rt_head->ri_count--;
    } else {
        chunk_free(rnode_chunk_header, rn);
        rt_head->rn_count--;
    }
}

/*
 * rtn_key_nextbit
 *
 * Check the (N+1)'th bit of a specified key where N = 0, 1, ....
 *
 * For example, when N is zero, we test the first bit of the key.
 */
static inline int8_t
rtn_key_nextbit (u_int8_t *key, u_int16_t bitlen)
{
    return (key[RNBYTE(bitlen)] & RNBIT(bitlen));
}

/*
 * rtn_key_cmp
 *
 * Compare the specified bits of two keys.
 */
static inline int8_t
rtn_key_cmp (u_int8_t *key1, u_int8_t *key2, u_int16_t bitlen)
{
    u_int8_t bits, mask;
    u_int16_t bytelen;

    bits = bitlen & 0x7;
    bytelen = bitlen >> 3;

    if (bits) {
        mask = 0xff << (8 - bits);
        if ((key1[bytelen] ^ key2[bytelen]) & mask)
            return (FALSE);
    }

    if (bytelen && memcmp(key1, key2, bytelen))
        return (FALSE);

    return (TRUE);
}

/*
 * rtn_root_init
 *
 * Initialize the root of a radix tree. This must be called first,
 * and the root be remembered by each tree user before calling other
 * radix tree API's.
 */
rt_node_t *
rtn_root_init (rt_head_t *rt_head, u_int8_t flags, rt_info_free func)
{
    rt_node_t *rn;

    /*
     * Make sure the head is zero'ed out.
     */
    memset(rt_head, 0, sizeof(rt_head_t));

    rt_head->ri_free = func;
    rt_head->flags   = flags;

    if (!rnode_chunk_header) {
        rnode_chunk_header = chunk_header_init(sizeof(rt_node_t),
                                      RNODE_CHUNK_ELEM, CHUNK_FLAGS_LOCK,
                                      "radix node");
        if (!rnode_chunk_header) {
            return (NULL);
        }
    }

    rn = rtn_node_alloc(rt_head);
    rt_head->root = rn;
    return (rn);
}

/*
 * rtn_root_free
 *
 * Free the root of a radix tree when the tree is deleted.
 */
void
rtn_root_free (rt_head_t *rt_head)
{
    if (rt_head->root) {
        rtn_node_free(rt_head, rt_head->root);
        rt_head->root = NULL;
    }
}

/*
 * rtn_node2info
 */
static inline rt_info_t *
rtn_node2info (rt_node_t *rn)
{
    return ((rn && (rn->rnode_flags & RNODE_INFO)) ?
            ((rt_info_t *) rn) : NULL);
}

/*
 * rtn_seach
 *
 * Perform the exact match for a given prefix.
 */
rt_info_t *
rtn_search (rt_head_t *rt_head, char *addr, u_int16_t bitlen)
{
    rt_node_t *rn;
    int8_t dir_r;

    /*
     * Search down the tree until we find a node which
     * has a bit number the same as ours.
     */
    for (rn = rt_head->root; rn && (rn->rnode_bit < bitlen);) {
        dir_r = rtn_key_nextbit((u_int8_t *)addr, rn->rnode_bit);
        rn = dir_r ? rn->rnode_right : rn->rnode_left;
    }

    /*
     * If we didn't find an exact bit length match, we're gone.
     * If there is no info on this node, we're gone too.
     */
    if (!rn || (rn->rnode_bit != bitlen) ||
        !(rn->rnode_flags & RNODE_INFO)) {
        return (NULL);
    }

    /*
     * So far so good.  Fetch the address and see if we have an
     * exact match.
     */
    if (rtn_key_cmp((u_int8_t *)addr, ((rt_info_t *)rn)->rninfo_key, bitlen)) {
        return ((rt_info_t *) rn);
    } else {
        return (NULL);
    }
}

/*
 * rtn_lookup_node
 *
 * Find the best match node that covers the specified prefix.
 * We follow the addr bit as far as we can that matches the addr,
 * and then backtrack to find the entry with a bitlen >= our
 * search key len.
 */
rt_node_t *
rtn_lookup_node (rt_head_t *rt_head, char *addr, u_int16_t bitlen)
{
    register rt_node_t *rn, *rn_next, *rn_last;
    register int8_t dir_r;
    /*
     * If bitlen == 0, then return the root of the tree
     */
    if (bitlen == 0) {
        return (rt_head->root);
    }
    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which is external.
     */
    for (rn = rt_head->root; rn; rn = rn_next) {
        if ((rn->rnode_bit >= bitlen) &&
            (rn->rnode_flags & RNODE_EXTERNAL)) {
            /*
             * If there is a match for the first "bitlen" bits,
             * Stop!  We got our node!!
             *
             */
            if (rtn_key_cmp((u_int8_t *) addr,
                            ((rt_info_t *)rn)->rninfo_key, bitlen)) {
                break;
            }
            /*
             * If there is no matching node for the first "bitlen" bits,
             * we will not find any other node in this subtree that matches
             * the first "bitlen" bits.
             */
            rn = NULL;
            break;
        }
        dir_r = rtn_key_nextbit((u_int8_t *) addr, rn->rnode_bit);
        rn_next = dir_r ? rn->rnode_right : rn->rnode_left;
    }
    if (!rn) {
        return (NULL);
    }
    /*
     * Now backtrack upto a node where its parent has a smaller bit-mask
     * than the key len we are searching for. This node is the root of
     * the subtree we should walk
     */
    for (rn_last = rn; rn_last != rt_head->root; rn_last = rn_last->rnode_parent) {
        if (rn_last->rnode_parent->rnode_bit < bitlen) {
            break;
        }
    }
    return(rn_last);
}

/*
 * rtn_lookup
 *
 * Find the best match that covers the specified prefix.
 * We follow the addr bit as far as we can, and then backtrack to
 * find the entry that matches the addr.
 */
rt_info_t *
rtn_lookup (rt_head_t *rt_head, char *addr, u_int16_t bitlen)
{
    rt_node_t  *rn, *rn_next;
    void       *node_key;
    int8_t     dir_r;

    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which has info attached.
     */
    rn = rt_head->root;
    while ((rn->rnode_bit < bitlen) || !(rn->rnode_flags & RNODE_EXTERNAL)) {
        dir_r = rtn_key_nextbit((u_int8_t *)addr, rn->rnode_bit);
        rn_next = dir_r ? rn->rnode_right : rn->rnode_left;
        if (!rn_next) {
            break;
        }
        rn = rn_next;
    }

    node_key = (rn->rnode_flags & RNODE_EXTERNAL) ?
        ((rt_info_t *) rn)->rninfo_key : NULL;
    /*
     * We should never get here, but if we do, return NULL
     */
    if (!node_key) {
        return (NULL);
    }

    /*
     * Backtrack to find the node.
     */
    for (; rn != rt_head->root; rn = rn->rnode_parent) {
        if ((rn->rnode_bit <= bitlen) &&
            rtn_key_cmp((u_int8_t *) addr, node_key, rn->rnode_bit) &&
            (rn->rnode_flags & RNODE_INFO)) {
            break;
        }
    }
    return (rtn_node2info(rn));
}

/*
 * rtn_lookup_range
 *
 *
 * Find the shortest match that is covered by the specified prefix/mask
 * We follow the addr bit as far as we can, and then backtrack to
 * find the entry that matches the addr.
 */
rt_info_t *
rtn_lookup_range (rt_head_t *rt_head, char *addr, u_int16_t maxbitlen)
{
    register rt_node_t *rn, *rn_next, *root;
    register int8_t dir_r;
    register rt_node_t *last_rn;

    root = rt_head->root;
    last_rn = NULL;

    for (rn = root; rn; rn = rn_next) {
        dir_r = rtn_key_nextbit((u_int8_t *)addr, rn->rnode_bit);
        rn_next = dir_r ? rn->rnode_right : rn->rnode_left;
        if (!rn_next)
            break;
    }

    for (; rn != root; rn = rn->rnode_parent) {
        if ((rn->rnode_flags & RNODE_INFO) && (rn->rnode_bit >= maxbitlen)) {
            if (rtn_key_cmp((u_int8_t *)addr, ((rt_info_t *)rn)->rninfo_key, maxbitlen)) {
                last_rn = rn;
            }
        }
    }

    return (rtn_node2info(last_rn));
}


/*
 * rtn_lookup_prefix_overlap()
 *
 * Find the closest node that either covers the specified prefix or
 * is covered by the specified prefix.
 * We follow the addr bit as far as it's an external node with rnode_bit
 * >= our search key len, if there is a match,
 * then backtrack to find the entry with a rnode_bit < our
 * search key len.
 */
rt_info_t *
rtn_lookup_prefix_overlap(rt_head_t *rt_head, char *addr, u_int16_t maxbitlen)
{
    register rt_node_t *rn, *rn_next, *rn_last = NULL;
    register int8_t dir_r;
    bool overlap_found = FALSE;

    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which is external.
     */
    for (rn = rt_head->root; rn; rn = rn_next) {
        if ((rn->rnode_bit >= maxbitlen) &&
            (rn->rnode_flags & RNODE_INFO)) {
            /*
             * check if the node is covered by the given prefix.
             */
            if (rtn_key_cmp((u_int8_t *) addr,
                            ((rt_info_t *)rn)->rninfo_key, maxbitlen)) {
                overlap_found = TRUE;
                break;
            }
        }
        dir_r = rtn_key_nextbit((u_int8_t *) addr, rn->rnode_bit);
        rn_next = dir_r ? rn->rnode_right : rn->rnode_left;
        rn_last = rn;
    }


    if(rn && overlap_found == TRUE) {
        return ((rt_info_t *)rn);
    }


    if(rn == NULL){
        rn = rn_last;
    }
    overlap_found = FALSE;

    /*
     * Now backtrack up to find if there is a node that covers the given prefix
     *
     */
    for (; rn != rt_head->root; rn = rn->rnode_parent) {
        if(rn->rnode_flags & RNODE_INFO){
            if (rtn_key_cmp((u_int8_t *) addr,
                            ((rt_info_t *)rn)->rninfo_key, rn->rnode_bit)) {
                overlap_found = TRUE;
                break;
            }
        }

    }

    if(rn && overlap_found == TRUE) {
        return ((rt_info_t *)rn);
    }

    return NULL;

}

/*
 * rtn_replace_node
 *
 * Replace a node by another node. Each node could be either internal
 * or external.
 *
 * (If we knew the info size, we could have just copied the new info.)
 */
static void
rtn_replace_node (rt_head_t *rt_head, rt_node_t *rn_old, rt_node_t *rn_new)
{
    rt_node_t *parent;

    /*
     * Copy the relevant info over.
     */
    rn_new->rnode_left    = rn_old->rnode_left;
    rn_new->rnode_right   = rn_old->rnode_right;
    rn_new->rnode_parent  = rn_old->rnode_parent;
    rn_new->rnode_bit     = rn_old->rnode_bit;
    rn_new->rnode_version = rn_old->rnode_version;

    parent = rn_old->rnode_parent;
    if (parent) {
        if (parent->rnode_left == rn_old) {
            parent->rnode_left = rn_new;
        } else {
            assert(parent->rnode_right == rn_old);
            parent->rnode_right = rn_new;
        }
    }

    if (rn_old->rnode_left) {
        rn_old->rnode_left->rnode_parent = rn_new;
    }

    if (rn_old->rnode_right) {
        rn_old->rnode_right->rnode_parent = rn_new;
    }

    if (rt_head->root == rn_old) {
        rt_head->root = rn_new;
    }

    /*
     * The old node is no longer in the tree, and should be deleted.
     * If it is locked, mark it for later free. Note this is a
     * different flag as the node is no longer in the tree.
     */
    if (rn_old->rnode_lock) {
        rn_old->rnode_flags |= RNODE_REPLACED;
    } else {
        rtn_node_free(rt_head, rn_old);
    }
}

/*
 * rtn_add
 *
 * Insert an info item into the tree (adapted from Gated/IENG).
 *
 * The following text is from GateD:
 *
 * When you install a route in the tree you attach it to an internal node
 * which has a bit number equal to the position of the first zero bit in
 * the route's mask (e.g. a default route is attached to an internal node
 * with bit number 0, while an IP host route is attached to an internal
 * node with bit number 32).  If you only deal with routes with contiguous
 * masks you will end up with no more than one route attached to any
 * internal node, otherwise you need to chain together all routes whose
 * addresses agree up the the first zero bit in the masks.  Some internal
 * nodes (<= half) will have no attached external information, I call
 * these "splits".  These are guaranteed to have both a left and right
 * child.  Nodes with external information may have zero, one or two
 * children.
 *
 * End of GateD quote.
 */
int8_t
rtn_add (rt_head_t *rt_head, rt_info_t *rinfo, u_int16_t bitlen)
{
    register rt_node_t *rn, *rn_prev, *rn_add, *rn_new;
    register u_short bits2chk, dbit;
    register u_char *addr, *his_addr;
    register u_int i;

    /*
     * just to be safe.
     */
    bzero(rinfo, sizeof(rt_node_t));

    rinfo->rnode_flags = (RNODE_INFO | RNODE_EXTERNAL);
    rinfo->rnode_bit = bitlen;
    rt_head->ri_count++;

    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which has info attached.  It
     * is possible we won't get down the tree this far, however,
     * so deal with that as well.
     */
    rn = rt_head->root;
    addr = rinfo->rninfo_key;
    while ((rn->rnode_bit < bitlen) || !(rn->rnode_flags & RNODE_EXTERNAL)) {
        if (addr[RNBYTE(rn->rnode_bit)] & RNBIT(rn->rnode_bit)) {
            if (!(rn->rnode_right)) {
                break;
            }
            rn = rn->rnode_right;
        } else {
            if (!(rn->rnode_left)) {
                break;
            }
            rn = rn->rnode_left;
        }
    }

    /*
     * Now we need to find the number of the first bit in our address
     * which differs from his address. Note that rn could be the root
     * without info..
     */
    bits2chk = MIN(rn->rnode_bit, bitlen);
    his_addr = (rn->rnode_flags & RNODE_EXTERNAL) ?
        ((rt_info_t *) rn)->rninfo_key : NULL;

    assert (his_addr || (bits2chk == 0));

    for (dbit = 0; dbit < bits2chk; dbit += RNBBY) {
        i = dbit >> RNSHIFT;
        if (addr[i] != his_addr[i]) {
            dbit += first_bit_set[addr[i] ^ his_addr[i]];
            break;
        }
    }

    /*
     * If the different bit is less than bits2chk we will need to
     * insert a split above him.  Otherwise we will either be in
     * the tree above him, or attached below him.
     */
    if (dbit > bits2chk) {
        dbit = bits2chk;
    }
    rn_prev = rn->rnode_parent;
    while (rn_prev && rn_prev->rnode_bit >= dbit) {
        rn = rn_prev;
        rn_prev = rn->rnode_parent;
    }

    /*
     * Okay.  If the node rn points at is equal to our bit number, we
     * may just be able to attach the info to him.  Check this since it
     * is easy.
     */
    if (dbit == bitlen && rn->rnode_bit == bitlen) {

        /*
         * Do not insert if the entry already exists.
         */
        if (rn->rnode_flags & RNODE_INFO) {
            rt_head->ri_count--;
            return(FALSE);
        }

        rtn_replace_node(rt_head, rn, (rt_node_t *) rinfo);
        return (TRUE);
    }

    /*
     * At least one new node needs to be added.
     */
    rn_add = (rt_node_t *) rinfo;

    /*
     * There are a couple of possibilities.  The first is that we
     * attach directly to the thing pointed to by rn.  This will be
     * the case if his bit is equal to dbit.
     */
    if (rn->rnode_bit == dbit) {
        assert(dbit < bitlen);

        rn_add->rnode_parent = rn;
        if (addr[RNBYTE(rn->rnode_bit)] & RNBIT(rn->rnode_bit)) {
            assert(!(rn->rnode_right));
            rn->rnode_right = rn_add;
        } else {
            assert(!(rn->rnode_left));
            rn->rnode_left = rn_add;
        }
        return (TRUE);
    }

    /*
     * The other case where we don't need to add a split is where
     * we were on the same branch as the guy we found.  In this case
     * we insert rn_add into the tree between rn_prev and rn.  Otherwise
     * we add a split between rn_prev and rn and append the node we're
     * adding to one side of it.
     */
    if (dbit == bitlen) {
        if (his_addr[RNBYTE(bitlen)] & RNBIT(bitlen)) {
            rn_add->rnode_right = rn;
        } else {
            rn_add->rnode_left = rn;
        }
        rn_new = rn_add;
    } else {
        rn_new = rtn_node_alloc(rt_head);
        if (!rn_new) {
            rt_head->ri_count--;
            return (FALSE);
        }

        rn_new->rnode_bit = dbit;
        rn_add->rnode_parent = rn_new;
        if (addr[RNBYTE(dbit)] & RNBIT(dbit)) {
            rn_new->rnode_right = rn_add;
            rn_new->rnode_left = rn;
        } else {
            rn_new->rnode_left = rn_add;
            rn_new->rnode_right = rn;
        }
    }
    rn->rnode_parent = rn_new;
    rn_new->rnode_parent = rn_prev;

    /*
     * If rn_prev is NULL this is a new root node, otherwise it
     * is attached to the guy above in the place where rn was.
     */
    if (!rn_prev) {
        ;
	/* rtt->rt_table_tree = rn_new; */
    } else if (rn_prev->rnode_right == rn) {
        rn_prev->rnode_right = rn_new;
    } else {
        assert(rn_prev->rnode_left == rn);
        rn_prev->rnode_left = rn_new;
    }

    /*
     * Update the rnode_version of the new midnode with that of
     * its child to ensure that the max. version number of its
     * subtree as recorded in rnode_version is still correct.
     */
    rn_new->rnode_version = rn->rnode_version;

    return (TRUE);
}

/*
 * rtn_delete_node
 *
 * Process the deletion of a node.
 */
static void
rtn_delete_node (rt_head_t *rt_head, rt_node_t *rn)
{
    rt_node_t *node, *child;

    for (; rn != rt_head->root; rn = node) {
        node = rn->rnode_parent;

        if (rn->rnode_flags & RNODE_INFO) {
            break;
        }

        /*
         * If a user is sleeping on this node, just markt it for
         * later check and deletion. Note that however, due to the
         * way rtn_add() is done, we can not leave any unnecessary
         * internal nodes in the tree.
         */
        if (rn->rnode_lock && (rn->rnode_flags & RNODE_EXTERNAL)) {
            rn->rnode_flags |= RNODE_DELETED;
            break;
        }

        /*
         * Do not delete a node with two children. Just release
         * the info.
         */
        if (rn->rnode_right && rn->rnode_left) {
            rn->rnode_flags &= ~RNODE_DELETED;
            if (rn->rnode_flags & RNODE_EXTERNAL) {
                node = rtn_node_alloc(rt_head);
                if (node) {
                    rtn_replace_node(rt_head, rn, node);
                }
            }
            break;
        }

        /*
         * This node has zero or one child, and should be removed
         * from the tree.
         */
        child = NULL;
        if (rn->rnode_left)
            child = rn->rnode_left;
        if (rn->rnode_right)
            child = rn->rnode_right;

        if (node->rnode_left == rn)
            node->rnode_left = child;
        else
            node->rnode_right = child;

        if (child) {
            child->rnode_parent = node;
        }

        /*
         * If the node is locked, just mark it for later free.
         */
        if (rn->rnode_lock) {
            rn->rnode_flags &= ~RNODE_DELETED;
            rn->rnode_flags |= RNODE_REPLACED;
        } else {
            rtn_node_free(rt_head, rn);
        }
    }
}

/*
 * rtn_delete
 *
 * Remove an info entry from the tree.
 * We support delayed deletion. Also, the root will not be deleted.
 */
void
rtn_delete (rt_info_t *rinfo, rt_head_t *rt_head)
{
    rt_node_t *rn, *node;

    rn = RADIX_INFO2NODE(rinfo);
    assert(rn && ((rn->rnode_flags & (RNODE_INFO | RNODE_EXTERNAL)) ==
                  (RNODE_INFO | RNODE_EXTERNAL)));

    /*
     * Delete the info flag immediately to avoid complications
     * by (delete, add) during yield.
     */
    rn->rnode_flags &= ~RNODE_INFO;

    /*
     * Do not delete the root. Also when another thread is sleeping
     * on this external node, just mark the node for later deletion.
     * Leaving the node temporarily in the tree would make it faster
     * for the other thread to find the next node.
     */
    if (rn == rt_head->root) {
        node = rtn_node_alloc(rt_head);
        if (node) {
            rtn_replace_node(rt_head, rn, node);
        }
    } else if (rn->rnode_lock) {
        rn->rnode_flags |= RNODE_DELETED;
    } else {
        rtn_delete_node(rt_head, rn);
    }
}

/*
 * rtn_bump_version_internal
 *
 * Set the info version and update
 * the max version of all its parents.
 *
 * Return TRUE if there is a version wrap.
 */
static inline int8_t
rtn_bump_version_internal(rt_info_t *rinfo, u_int32_t version)
{
    rt_node_t *rn;

    rn = RADIX_INFO2NODE(rinfo);
    assert(rn != NULL);

    rinfo->version = version;
    while (rn) {
        rn->rnode_version = version;
        rn = rn->rnode_parent;
    }

    return ((version == 0) ? TRUE : FALSE);
}

/*
 * rtn_bump_version
 *
 * Bump the table version, and then set the info version and update
 * the max version of all its parents.
 *
 * Return TRUE if there is a version wrap.
 */
int8_t
rtn_bump_version (rt_info_t *rinfo, u_int32_t *table_version)
{

    /*
     * Check if any of the arguments is a null pointer
     * We should have better check for null pointers
     * at other places too. This is just a start.
     */
    if ((!rinfo) || (!table_version)) {
	return(FALSE);
    }
    (*table_version)++;
    return (rtn_bump_version_internal(rinfo, *table_version));
}

/*
 * rtn_bump_version_max_limit
 *
 * Bump the table version, and set to zero once we pass max requested
 * and then set the info version and update
 * the version of all its parents.
 *
 * Return TRUE if there is a version wrap.
 */
int8_t
rtn_bump_version_max_limit (rt_info_t *rinfo, u_int32_t *table_version, u_int32_t max)
{
    /*
     * Check if any of the arguments is a null pointer
     * We should have better check for null pointers
     * at other places too. This is just a start.
     */
    if ((!rinfo) || (!table_version)) {
	return(FALSE);
    }

    (*table_version)++;
    if (*table_version > max) {
        *table_version = 0;
    }
    return (rtn_bump_version_internal(rinfo, *table_version));
}

/*
 * rtn_walk_descend
 *
 * Return the next node when we walk down.
 */
static inline rt_node_t *
rtn_walk_descend (rt_node_t *rn)
{
    return (rn->rnode_left ? rn->rnode_left : rn->rnode_right);
}

/*
 * rtn_walk_ascend
 *
 * We climb up the tree when we can no longer walk down. In climbing up,
 * we try to figure out the next node.
 *
 * For example, the successor node of (3) is (4), and the successor node
 * of (4) is (5) in this figure.
 *
 *             1
 *            / \
 *           2   5
 *          / \
 *         3   4
 *
 * The next node must be below the subtree node (when specified).
 */
rt_node_t *
rtn_walk_ascend (rt_node_t *st_root, rt_node_t *rn)
{
    rt_node_t *node;

    /*
     * When climbing up from the right, we must continue climbing
     * until we climb up from the left. When climbing up from the
     * left, we must find a node with a right child.
     */
    while (rn && (rn != st_root)) {
        node = rn;
        rn = rn->rnode_parent;

        if (rn && (rn->rnode_left == node) && rn->rnode_right) {
            return (rn->rnode_right);
        }
    }

    return (NULL);
}

/*
 * rtn_walk_next_node
 *
 * Get the next node (below the subtree if specified).
 */
rt_node_t *
rtn_walk_next_node (rt_node_t *st_root, rt_node_t *rn)
{
    rt_node_t *node;

    if (!rn) {
        return (NULL);
    }

    node = rtn_walk_descend(rn);
    if (!node) {
        node = rtn_walk_ascend(st_root, rn);
    }
    return (node);
}

/*
 * rtn_walk_next_info
 *
 * Get the next info (below the subtree if specified).
 */
static inline rt_info_t *
rtn_walk_next_info (rt_node_t *st_root, rt_node_t *rn)
{
    rt_node_t *node;

    node = rn;
    while ((node = rtn_walk_next_node(st_root, node)) != NULL) {
        if (node->rnode_flags & RNODE_INFO) {
            return ((rt_info_t *) node);
        }
    }

    return (NULL);
}

/*
 * rtn_check_subtree_root
 *
 * Check if the specified key still falls in the subtree.
 */
static int
rtn_check_subtree_root (rt_node_t *st_root, char *addr, u_int16_t bitlen)
{
    rt_node_t  *rn;
    u_char     *st_addr;

    if (st_root->rnode_bit > bitlen) {
        return (-1);
    }

    for (rn = st_root; rn && !(rn->rnode_flags & RNODE_EXTERNAL);) {
        rn = rtn_walk_next_node(st_root, rn);
    }
    assert(rn);
    st_addr = ((rt_info_t *) rn)->rninfo_key;
    if (!rtn_key_cmp((u_int8_t *)st_addr, (u_int8_t *)addr, st_root->rnode_bit)) {
        return (-1);
    }

    return (0);
}

/*
 * rtn_key_getnext_node
 *
 * For a given address and bitlen, find the next lexical info entry
 * that is below the subtree node.
 *
 * The approach here is similar to rtn_add(). If we know where the
 * entry should be added, then it is easier to figure our what entry
 * is the next one.
 *
 * When exact_match is TRUE, return the exact_match entry if it exists.
 * This is a pre-order walk, and returns the pre-order next.
 */
static rt_node_t *
rtn_key_getnext_node (rt_head_t *rt_head, rt_node_t *st_root, u_char *addr,
                      u_int16_t bitlen, int exact_match)
{
    register rt_node_t *rn, *rn_prev;
    register u_short bits2chk, dbit;
    register u_char *his_addr;
    register u_int i;

    /*
     * If the stree_node is not the root, check if it is still
     * above the given address.
     */
    if (st_root && (st_root != rt_head->root)) {
        if (rtn_check_subtree_root(st_root, (char *)addr, bitlen) < 0) {
            return (NULL);
        }

        if (exact_match && (st_root->rnode_bit == bitlen)) {
            return (st_root);
        }
    }

    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which has info attached.  It
     * is possible we won't get down the tree this far, however,
     * so deal with that as well.
     */
    rn = rt_head->root;
    while ((rn->rnode_bit < bitlen) || !(rn->rnode_flags & RNODE_EXTERNAL)) {
        if (addr[RNBYTE(rn->rnode_bit)] & RNBIT(rn->rnode_bit)) {
            if (!(rn->rnode_right)) {
                break;
            }
            rn = rn->rnode_right;
        } else {
            if (!(rn->rnode_left)) {
                break;
            }
            rn = rn->rnode_left;
        }
    }

    /*
     * Now we need to find the number of the first bit in our address
     * which differs from his address.
     */
    bits2chk = MIN(rn->rnode_bit, bitlen);
    his_addr = (rn->rnode_flags & RNODE_EXTERNAL) ?
        ((rt_info_t *) rn)->rninfo_key : NULL;

    assert (his_addr || (bits2chk == 0));

    for (dbit = 0; dbit < bits2chk; dbit += RNBBY) {
        i = dbit >> RNSHIFT;
        if (addr[i] != his_addr[i]) {
            dbit += first_bit_set[addr[i] ^ his_addr[i]];
            break;
        }
    }

    /*
     * If the different bit is less than bits2chk we will need to
     * insert a split above him.  Otherwise we will either be in
     * the tree above him, or attached below him.
     */
    if (dbit > bits2chk) {
        dbit = bits2chk;
    }
    rn_prev = rn->rnode_parent;
    while (rn_prev && rn_prev->rnode_bit >= dbit) {
        rn = rn_prev;
        rn_prev = rn->rnode_parent;
    }

    /*
     * Okay.  If the node rn points at is equal to our bit number, we
     * may just be able to attach the info to him.  Check this since it
     * is easy.
     */
    if (dbit == bitlen && rn->rnode_bit == bitlen) {
        if (!exact_match) {
            rn = rtn_walk_next_node(st_root, rn);
        }
        return (rn);
    }

    /*
     * There are a couple of possibilities.  The first is that we
     * attach directly to the thing pointed to by rn.  This will be
     * the case if his bit is equal to dbit.
     */
    if (rn->rnode_bit == dbit) {
        assert(dbit < bitlen);

        if (addr[RNBYTE(rn->rnode_bit)] & RNBIT(rn->rnode_bit)) {
            assert(!(rn->rnode_right));
            rn = rtn_walk_ascend(st_root, rn);
        } else {
            assert(!(rn->rnode_left));
            rn = rtn_walk_next_node(st_root, rn);
        }

        return (rn);
    }

    /*
     * The other case where we don't need to add a split is where
     * we were on the same branch as the guy we found.  In this case
     * we insert rn_add into the tree between rn_prev and rn.  Otherwise
     * we add a split between rn_prev and rn and append the node we're
     * adding to one side of it.
     */
    if (dbit == bitlen) {
        return (rn);
    } else {
        if (addr[RNBYTE(dbit)] & RNBIT(dbit)) {
            rn = rtn_walk_ascend(st_root, rn);
        }

        return (rn);
    }
}

/*
 * rtn_key_getnext
 *
 * For a given address and bitlen, find the next lexical info entry
 * in the tree.
 */
rt_info_t *
rtn_key_getnext (rt_head_t *rt_head, char *addr, u_int16_t bitlen,
                 int exact_match)
{
    rt_node_t *rn;

    rn = rtn_key_getnext_node(rt_head, NULL, (u_char *)addr, bitlen, exact_match);
    if (rn) {
        if (rn->rnode_flags & RNODE_INFO) {
            return ((rt_info_t *) rn);
        } else {
            return (rtn_walk_next_info(NULL, rn));
        }
    }
    return (NULL);
}

/*
 * rtn_update_subtree_info
 *
 * Check and update the subtree info after yield.
 */
static rt_node_t *
rtn_update_subtree_info (rt_head_t *rt_head, stree_info_t *st_info)
{
    rt_node_t *st_root;

    st_root = *(st_info->st_root);
    if (st_root->rnode_flags & RNODE_REPLACED) {
        if (--st_root->rnode_lock == 0) {
            rtn_node_free(rt_head, st_root);
        }

        st_root = rtn_lookup_node(rt_head, st_info->st_addr, st_info->st_bitlen);
        if (!st_root) {
            return (NULL);
        }
        st_root->rnode_lock++;
        *(st_info->st_root) = st_root;
    }

    return (st_root);
}

/*
 * rtn_check_node_deletion
 *
 * Check and delete a node after its lock is releases.
 */
static inline void
rtn_check_node_deletion (rt_head_t *rt_head, rt_node_t *rn)
{
    if (rn->rnode_flags & RNODE_REPLACED) {
        rtn_node_free(rt_head, rn);
    } else if (rn->rnode_flags & RNODE_DELETED) {
        rtn_delete_node(rt_head, rn);
    }
}

/*
 * rtn_process_external_node
 *
 * Process an external node, and return the next node.
 */
static rt_node_t *
rtn_process_external_node (rt_head_t *rt_head, stree_info_t *st_info,
                           rt_node_t *rn, int blocking, rtn_walk_func fi,
                           struct timeval *tval, int *errnum, va_list ap)
{
    rt_node_t *node, *st_root;
    rt_info_t *ri;

    /*
     * Lock this node so that it does not get deleted when we yield
     * in processing the node. This will allow us to find the next
     * node. Also by having this external node in the tree will ensure
     * the existance of the external info necessary for additions and
     * getnext lookups during the yield.
     */
    rn->rnode_lock++;

    ri = (rt_info_t *) rn;
    *errnum = (*fi)(ri, ap);

    switch (blocking) {
      case 0:
        break;
      case 1:
        pthread_may_yield(NULL, &rtn_wc_tv);
        break;
      case 2:
        pthread_may_yield(NULL, tval);
        break;
      default:
        pthread_may_yield(tval, &rtn_wc_tv);
    }

    /*
     * Check and update the subtree info after the yield.
     */
    if (st_info) {
        st_root = rtn_update_subtree_info(rt_head, st_info);
    } else {
        st_root = NULL;
    }

    /*
     * Identify the next node before the lock is released. Note that
     * the next node identified will either be below, or off a different
     * branch (that is, will not be directly above the node that might
     * need to be deleted).
     *
     * More work is required if the node is no longer in the tree (due
     * to possible deletion and addition during yield)
     */
    if (rn->rnode_flags & RNODE_REPLACED) {
        node = rtn_key_getnext_node(rt_head, st_root, (u_char *)ri->rninfo_key,
                                    ri->rnode_bit, FALSE);
    } else {
        node = rtn_walk_next_node(st_root, rn);
    }

    /*
     * Release the lock we placed, and process deletion marked when
     * we yield and update the root of the subtree.
     */
    if (--rn->rnode_lock == 0) {
        rtn_check_node_deletion(rt_head, rn);
        if (st_info) {
            rtn_update_subtree_info(rt_head, st_info);
        }
    }

    return (node);
}

/*
 * rtn_thread_time
 *
 * Make sure a given value is sane.
 */
static u_int32_t
rtn_thread_time (u_int32_t in_value)
{
#define     RTN_THREAD_TIME_DEFAULT  100   /* ms, default */
#define     RTN_THREAD_TIME_MIN       50   /* ms, minimum */
#define     RTN_THREAD_TIME_MAX      300   /* ms, maximum */

    if ((in_value < RTN_THREAD_TIME_MIN) ||
        (in_value > RTN_THREAD_TIME_MAX)) {
        return (RTN_THREAD_TIME_DEFAULT);
    }

    return (in_value);
}

/*
 * This is a non-recursive, pre-order tree traversal in which a parent
 * node is processed first, followed by its left child, and its right
 * child.
 *
 * Here is an example of pre-order:
 *
 *                 1
 *                / \
 *               2   7
 *              / \   \
 *             3   4   8
 *                / \
 *               5   6
 *
 * The "lexical order" for routing table (in show command, and SNMP) can
 * be achieved by a pre-order walk.
 *
 * In this code, each node is touched twice - one in walking down, and
 * one in climbing up. The info of a node is processed when we try to
 * walk down. In walking down, we try to go to the left child first,
 * and then its right child. If we can no longer go down (i.e., reach
 * a node without any child), we will then climb up. If we climb up
 * from a right child, we may need to climb multiple nodes.
 *
 * As we do pre-order walk, we actually can start the walk from a node
 * other than the root, and still process the nodes that are lexically
 * larger than the given start_node. This is useful if we perform limited
 * number of entries in each walk, and require multiple walks to visit
 * all the nodes.
 *
 * The thread yields after it has run for a pre-specified duration.
 */
int8_t
rtn_walktree (rt_node_t *start_node, rtn_walk_func fi, rt_head_t *rt_head,
              u_int32_t limit_ms, int8_t blocking, ...)
{
    rt_node_t *rn, *node;
    int errnum = RTWALK_CONTINUE;
    struct timeval tval;
    va_list ap;

    rt_head->rtn_walktree_count++;

    /*
     * Start from the root when the start node is not given.
     */
    if (!start_node) {
        start_node = rt_head->root;
    }

    /*
     * Note that limit is in ms.
     */
    tval.tv_sec = 0;
    tval.tv_usec = rtn_thread_time(limit_ms) * 1000;

    for (rn = start_node; rn; rn = node) {

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Process the external node.
         */
        va_start(ap, blocking);
        node = rtn_process_external_node(rt_head, NULL, rn, blocking, fi,
                                         &tval, &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            break;
        }
    }

    return (errnum);
}

/*
 * rtn_walktree_version
 *
 * Perform versioned pre-order tree traversal.
 *
 * Here is how it works: the rnode_version of a node is the max. version
 * number of all its chilren. We first compare the rnode_version with
 * a user specified version number to decide if we need to walk down
 * (including the current node). If "yes", we also need to compare
 * the version of the info entry with the user specified number to
 * decide if we need to process the info entry.
 *
 * To make this work, the routine rtn_bump_version() must be called
 * each time there is a new version for an info entry.
 */
int8_t
rtn_walktree_version (rt_node_t *start_node, rtn_walk_func fi,
                      rt_head_t *rt_head, u_int32_t min_version,
                      u_int32_t max_version, u_int32_t limit_ms,
                      int8_t blocking, ...)
{
    rt_node_t *rn, *node;
    rt_info_t *ri;
    int errnum = RTWALK_CONTINUE;
    struct timeval tval;
    va_list ap;

    rt_head->rtn_walktree_version_count++;
    /*
     * Start from the root when the start node is not given.
     */
    if (!start_node) {
        start_node = rt_head->root;
    }

    /*
     * Note that limit is in ms.
     */
    tval.tv_sec = 0;
    tval.tv_usec = rtn_thread_time(limit_ms) * 1000;

    for (rn = start_node; rn; rn = node) {

        /*
         * First check the version condition. If the max. version of the
         * subtree is not larger than the min version, do not go down
         * further.
         */
        if (rn->rnode_version <= min_version) {
            node = rtn_walk_ascend(NULL, rn);
            continue;
        }

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Skip if the info version is outside the range.
         */
        ri = (rt_info_t *) rn;
        if ((ri->version <= min_version) ||
            (!(rt_head->flags & RTN_BIT_MULTI_INFO) &&
             (ri->version > max_version))) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Process the external node.
         */
        va_start(ap, blocking);
        node = rtn_process_external_node(rt_head, NULL, rn, blocking, fi,
                                         &tval, &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            break;
        }
    }

    return (errnum);
}


/*
 * rtn_walktree_limited_curnext
 *
 * Walk a specified number of info entries, and then return.
 * Record the last info entry that a user should resume processing.
 * Return the last processed as well as the next unprocessed nodes.
 * If the walk was stopped for non-app requested reasons, then both
 * cur_item and next_item may point to the same element.
 */
int8_t
rtn_walktree_limited_curnext (rt_node_t *start_node, rtn_walk_func fi,
                      rt_info_t **next_item, rt_info_t **cur_item,
		      rt_head_t *rt_head, u_int32_t limit, ...)
{
    rt_node_t *rn, *node;
    u_int32_t item_done = 0;
    int errnum = RTWALK_CONTINUE;
    va_list ap;

    if (!start_node) {
        start_node = rt_head->root;
    }

    *next_item = NULL;
    *cur_item = NULL;

    for (rn = start_node; rn; rn = node) {

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Check the limit.
         */
        if (item_done++ >= limit) {
            errnum = RTWALK_ABORT;
            *next_item = (rt_info_t *) rn;
            *cur_item = (rt_info_t *) rn;
            break;
        }

        /*
         * Process the external node.
         */
        va_start(ap, limit);
        node = rtn_process_external_node(rt_head, NULL, rn, FALSE, fi, NULL,
                                         &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            errnum = RTWALK_ABORT;
	    /* Done processing current node - rn
	     * Return the "node" for further processing
	     */
            *next_item = (rt_info_t *) node;
            *cur_item = (rt_info_t *) rn;
            break;
        }
    }

    return (errnum);
}


/*
 * rtn_walktree_limited
 *
 * Walk a specified number of info entries, and then return.
 * Record the last info entry that a user should resume processing.
 * (That is, this entry has not been processed.)
 */
int8_t
rtn_walktree_limited (rt_node_t *start_node, rtn_walk_func fi,
                      rt_info_t **last_item, rt_head_t *rt_head,
                      u_int32_t limit, ...)
{
    rt_node_t *rn, *node;
    u_int32_t item_done = 0;
    int errnum = RTWALK_CONTINUE;
    va_list ap;

    rt_head->rtn_walktree_count++;

    if (!start_node) {
        start_node = rt_head->root;
    }

    *last_item = NULL;

    for (rn = start_node; rn; rn = node) {

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Check the limit.
         */
        if (item_done++ >= limit) {
            errnum = RTWALK_ABORT;
            *last_item = (rt_info_t *) rn;
            break;
        }

        /*
         * Process the external node.
         */
        va_start(ap, limit);
        node = rtn_process_external_node(rt_head, NULL, rn, FALSE, fi, NULL,
                                         &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            errnum = RTWALK_ABORT;
            *last_item = (rt_info_t *) rn;
            break;
        }
    }

    return (errnum);
}

/*
 * rtn_walktree_version_limited
 *
 * Versioned walk of a limited number of info entries, and return. Otherwise,
 * this routine would be the same as rtn_walktree_version().
 */
int8_t
rtn_walktree_version_limited (rt_node_t *start_node, rtn_walk_func fi,
                              rt_info_t **last_item, rt_head_t *rt_head,
                              u_int32_t min_version, u_int32_t max_version,
                              u_int32_t limit, ...)
{
    rt_node_t *rn, *node;
    rt_info_t *ri;
    u_int32_t item_done = 0;
    int errnum = RTWALK_CONTINUE;
    va_list ap;

    rt_head->rtn_walktree_version_count++;

    if (!start_node) {
        start_node = rt_head->root;
    }

    *last_item = NULL;

    for (rn = start_node; rn; rn = node) {

        /*
         * First check the version condition. If the max. version of the
         * subtree is not larger than the min version, do not go down
         * further.
         */
        if (rn->rnode_version <= min_version) {
            node = rtn_walk_ascend(NULL, rn);
            continue;
        }

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Skip if the info version is outside the range.
         */
        ri = (rt_info_t *) rn;
        if ((ri->version <= min_version) ||
            (!(rt_head->flags & RTN_BIT_MULTI_INFO) &&
             ri->version > max_version)) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Check the limit.
         */
        if (item_done++ >= limit) {
            errnum = RTWALK_ABORT;
            *last_item = (rt_info_t *) rn;
            break;
        }

        /*
         * Process the external node.
         */
        va_start(ap, limit);
        node = rtn_process_external_node(rt_head, NULL, rn, FALSE, fi, NULL,
                                         &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            errnum = RTWALK_ABORT;
            *last_item = (rt_info_t *) rn;
            break;
        }
    }

    return (errnum);
}

/*
 * rtn_dump_node
 *
 * This is used in pre-order tree traversal. We process a node when
 * we walk down. The next-node for walk is also returned.
 *
 * The thread suspends after a pre-specified number of entries have
 * been processed.
 */
static rt_node_t *
rtn_dump_node (rt_head_t *rt_head, rt_node_t *rn, int8_t blocking,
               rtn_walk_func fi, rnode_walk_func fn, struct timeval *tval,
               int *errnum, va_list ap)
{
    rt_node_t *node;

    /*
     * Verify the integrity of the tree, and display the node and info
     * when we walk down.
     */
    if (fn) {
        if (rn->rnode_left) {
            assert(rn->rnode_left->rnode_parent == rn);
        }

        if (rn->rnode_right) {
            assert(rn->rnode_right->rnode_parent == rn);
        }

        if (rn->rnode_parent) {
            assert((rn->rnode_parent->rnode_left == rn) ||
                   (rn->rnode_parent->rnode_right == rn));
        }

        (*fn)(rn);
    }

    if (rn->rnode_flags & RNODE_INFO) {
        node = rtn_process_external_node(rt_head, NULL, rn, blocking, fi, tval,
                                         errnum, ap);
    } else {
        node = rtn_walk_next_node(NULL, rn);
    }

    return (node);
}

/*
 * rtn_dumptree
 *
 * Walk the tree and dump its content, including hex pointers. Two
 * functions fi and fn must be supplied to display the info and node.
 *
 * Note that we could revise rtn_walktree() to accomodate this
 * functionality. But it would have involved too many user files.
 */
int8_t
rtn_dumptree (rt_node_t *start_node, rtn_walk_func fi, rnode_walk_func fn,
              rt_head_t *rt_head, u_int32_t limit_ms, int8_t blocking, ...)
{
    rt_node_t *rn, *node;
    int errnum = RTWALK_CONTINUE;
    struct timeval tval;
    va_list ap;

    if (!start_node) {
        start_node = rt_head->root;
    }

    /*
     * Note that limit is in ms.
     */
    tval.tv_sec = 0;
    tval.tv_usec = rtn_thread_time(limit_ms) * 1000;

    for (rn = start_node; rn; rn = node) {

        va_start(ap, blocking);
        node = rtn_dump_node(rt_head, rn, blocking, fi, fn, &tval, &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            break;
        }
    }

    return (errnum);
}

/*
 * rtn_walk_subtree
 *
 * Perform the subtree walk.
 */
int8_t
rtn_walk_subtree (rt_head_t *rt_head, char *st_addr, u_int16_t st_bitlen,
                  rtn_walk_func fi, u_int32_t limit_ms, int8_t blocking, ...)
{
    rt_node_t *st_root, *rn, *node;
    int errnum = RTWALK_CONTINUE;
    struct timeval tval;
    stree_info_t st_info;
    va_list ap;

    rt_head->rtn_walktree_count++;

    /*
     * First search for the most specific node that covers the
     * specified prefix. Note the subtree of this node may have
     * more than we want when the specified prefix does not exist.
     * We must compare the keys for each entry in that case.
     */
    st_root = rtn_lookup_node(rt_head, st_addr, st_bitlen);
    if (!st_root) {
        return (errnum);
    }

    /*
     * Lock the subtree root so that it does not get deleted
     * while we still need to use it.
     */
    st_root->rnode_lock++;

    /*
     * Note that limit is in ms.
     */
    tval.tv_sec = 0;
    tval.tv_usec = rtn_thread_time(limit_ms) * 1000;

    for (rn = st_root; rn; rn = node) {

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(st_root, rn);
            continue;
        }

        /*
         * Process the external node.
         */
        st_info.st_root     = &st_root;
        st_info.st_addr     = st_addr;
        st_info.st_bitlen   = st_bitlen;

        va_start(ap, blocking);
        node = rtn_process_external_node(rt_head, &st_info, rn, blocking, fi,
                                         &tval, &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            break;
        }
    }

    /*
     * Release the lock placed on the subtree root.
     */
    if (--st_root->rnode_lock == 0) {
        rtn_check_node_deletion(rt_head, st_root);
    }

    return (errnum);
}

/*
 * rtn_walk_subtree_limited
 *
 * Walk a specified number of info entries of a subtree and return.
 * Record the last info entry that a user should resume processing.
 * (That is, this entry has not been processed.)
 *
 * The subtree is identified by addr/bitlen, and the previous node
 * where the walk stopped is specified by prev_addr/prev_bitlen.
 * For the first round of walk, prev_bitlen is 0. exact is passed
 * directly to rtn_key_getnext(); if TRUE is will start at 'prev'
 * if there is an exact match, if FALSE is will start at the next
 * entry after 'prev'
 *
 * The whole tree has been walked when the *last_item is set to NULL.
 */
int8_t
rtn_walk_subtree_limited (rt_head_t *rt_head, char *st_addr, u_int16_t st_bitlen,
                          char *prev_addr, u_int16_t prev_bitlen,
                          rtn_walk_func fi, rt_info_t **last_item,
                          bool exact, u_int32_t limit, ...)
{
    rt_node_t *st_root, *rn, *node;
    u_int32_t item_done = 0;
    int errnum = RTWALK_CONTINUE;
    stree_info_t st_info;
    va_list ap;

    *last_item = NULL;

    /*
     * First search for the most specific node that covers the
     * specified prefix. Note the subtree of this node may have
     * more than we want when the specified prefix does not exist.
     * We must compare the keys for each entry in that case.
     */
    st_root = rtn_lookup_node(rt_head, st_addr, st_bitlen);
    if (!st_root) {
        return (errnum);
    }

    /*
     * Lock the subtree root so that it does not get deleted
     * while we still need to use it.
     */
    st_root->rnode_lock++;

    /*
     * Search for the next node to start the walk.
     */
    if (!prev_bitlen || !prev_addr) {
        rn = st_root;
    } else {
        rn = rtn_key_getnext_node(rt_head, st_root, (u_char*)prev_addr, prev_bitlen,
                                  exact);
    }

    for (; rn; rn = node) {

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(st_root, rn);
            continue;
        }

        /*
         * Check the limit.
         */
        if (item_done++ >= limit) {
            errnum = RTWALK_ABORT;
            *last_item = (rt_info_t *) rn;
            break;
        }

        /*
         * Process the external node.
         */
        st_info.st_root     = &st_root;
        st_info.st_addr     = st_addr;
        st_info.st_bitlen   = st_bitlen;

        va_start(ap, limit);
        node = rtn_process_external_node(rt_head, &st_info, rn, FALSE, fi,
                                         NULL, &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            errnum = RTWALK_ABORT;
            *last_item = (rt_info_t *) rn;
            break;
        }
    }

    /*
     * Release the lock placed on the subtree root.
     */
    if (--st_root->rnode_lock == 0) {
        rtn_check_node_deletion(rt_head, st_root);
    }

    return (errnum);
}

/*
 * rtn_walk_subtree_version
 *
 * Perform the versioned subtree walk.
 *
 * Note the subtree root specified by st_addr/st_bitlen may not exist.
 */
int
rtn_walk_subtree_version (rt_head_t *rt_head, u_char *st_addr,
                          u_int16_t st_bitlen, u_char *last_addr,
                          u_int16_t last_bitlen, rtn_walk_func fi,
                          u_int32_t min_version, u_int32_t max_version,
                          u_int32_t limit_ms, int8_t blocking, ...)
{
    rt_node_t  *st_root, *rn, *node;
    int        errnum = RTWALK_CONTINUE;
    struct     timeval tval;
    stree_info_t st_info;
    rt_info_t  *ri;
    va_list    ap;

    rt_head->rtn_walktree_version_count++;

    /*
     * First search for the most specific node that covers the
     * specified prefix. Note the subtree of this node may have
     * more than we want when the specified prefix does not exist.
     * We must compare the keys for each entry in that case.
     */
    st_root = rtn_lookup_node(rt_head, (char *) st_addr, st_bitlen);
    if (!st_root) {
        return (errnum);
    }
    if (st_root->rnode_version <= min_version) {
        return (errnum);
    }

    /*
     * If the last address/bitlen is specified, start from there.
     */
    if (!last_addr || (last_bitlen == 0)) {
        rn = st_root;
    } else {
        rn = rtn_key_getnext_node(rt_head, st_root, last_addr, last_bitlen,
                                  TRUE);
    }

    /*
     * Lock the subtree root so that it does not get deleted
     * while we still need to use it.
     */
    st_root->rnode_lock++;

    /*
     * Note that limit is in ms.
     */
    tval.tv_sec = 0;
    tval.tv_usec = rtn_thread_time(limit_ms) * 1000;

    for (; rn; rn = node) {

        /*
         * First check the version condition. If the max. version of the
         * subtree is not larger than the min version, do not go down
         * further.
         */
        if (rn->rnode_version <= min_version) {
            node = rtn_walk_ascend(st_root, rn);
            continue;
        }

        /*
         * Skip the internal node.
         */
        if (!(rn->rnode_flags & RNODE_INFO)) {
            node = rtn_walk_next_node(st_root, rn);
            continue;
        }

        /*
         * Skip if the info version is outside the range.
         */
        ri = (rt_info_t *) rn;
        if ((ri->version <= min_version) ||
            (!(rt_head->flags & RTN_BIT_MULTI_INFO) &&
             ri->version > max_version)) {
            node = rtn_walk_next_node(st_root, rn);
            continue;
        }

        /*
         * Process the external node.
         */
        st_info.st_root     = &st_root;
        st_info.st_addr     = (char *)st_addr;
        st_info.st_bitlen   = st_bitlen;

        va_start(ap, blocking);
        node = rtn_process_external_node(rt_head, &st_info, rn, blocking, fi,
                                         &tval, &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            break;
        }
    }

    /*
     * Release the lock placed on the subtree root.
     */
    if (--st_root->rnode_lock == 0) {
        rtn_check_node_deletion(rt_head, st_root);
    }

    return (errnum);
}

/*
 * rtn_walktree_keybits
 *
 * This function walks entries with keybits between (min_keybits,
 * max_keybits). When the specified max_keybits is smaller than the
 * max bits of the tree, this function is more efficient than walking
 * the entire tree. The arguments of this function are similar to those
 * in rtn_walktree_version().
 */
int8_t
rtn_walktree_keybits (rt_node_t *start_node, rtn_walk_func fi,
                      rt_head_t *rt_head, u_int32_t min_keybits,
                      u_int32_t max_keybits, u_int32_t limit_ms,
                      int8_t blocking, ...)
{
    rt_node_t *rn, *node;
    int errnum = RTWALK_CONTINUE;
    struct timeval tval;
    va_list ap;

    /*
     * Start from the root when the start node is not given.
     */
    if (!start_node) {
        start_node = rt_head->root;
    }

    /*
     * Note that limit is in ms.
     */
    tval.tv_sec = 0;
    tval.tv_usec = rtn_thread_time(limit_ms) * 1000;

    for (rn = start_node; rn; rn = node) {

        /*
         * First check the keybits condition. If the node bit is larger
         * than the max keybits, do not go down further.
         */
        if (rn->rnode_bit > max_keybits) {
            node = rtn_walk_ascend(NULL, rn);
            continue;
        }

        /*
         * Go to the next node if the node bit is too small.
         */
        if (!(rn->rnode_flags & RNODE_INFO) || (rn->rnode_bit < min_keybits)) {
            node = rtn_walk_next_node(NULL, rn);
            continue;
        }

        /*
         * Process the external node.
         */
        va_start(ap, blocking);
        node = rtn_process_external_node(rt_head, NULL, rn, blocking, fi,
                                         &tval, &errnum, ap);
        va_end(ap);

        if (errnum == RTWALK_ABORT) {
            break;
        }
    }

    return (errnum);
}

/*
 * rtn_detach_subtree
 *
 * Find and detach the subtree with given prefix string and len
 * from the given tree.
 */
rt_node_t *
rtn_detach_subtree (rt_head_t *rt_head, char *prefix, u_int32_t bitlen)
{
    rt_node_t *st_root;
    rt_node_t *st_root_parent;

    /* Find subtree root node */
    st_root = rtn_lookup_node(rt_head, prefix, bitlen);
    if (!st_root) {
        /* Return in case of not found */
        return (NULL);
    }

    /* Detach sub-tree from main tree */
    st_root_parent = st_root->rnode_parent;
    if (!st_root_parent) {
        /*
         * Subtree root node is main tree root node.
         * Replace main tree root node with internal node
         * and return replaced root node.
         */
        rt_head->root = rtn_node_alloc(rt_head);
        return (st_root);
    } else if (st_root_parent->rnode_right == st_root) {
        st_root_parent->rnode_right = NULL;
    } else {
        assert(st_root_parent->rnode_left == st_root);
        st_root_parent->rnode_left = NULL;
    }
    /* st_root is orphan now */
    st_root->rnode_parent = NULL;
    /* Clean up the main tree for any redundant splitter node */
    rtn_delete_node(rt_head, st_root_parent);

    return (st_root);
}

/*
 * rtn_attach_subtree
 *
 * Attach the subtree with given prefix string and len to the
 * main tree. Return error if subtree is already part of main tree.
 */
int8_t
rtn_attach_subtree (rt_head_t *rt_head, rt_node_t *st_root,
        char *addr, u_int32_t bitlen)
{
    rt_node_t *rn, *rn_prev, *rn_add, *rn_new;
    u_short bits2chk, dbit;
    char *his_addr;
    u_int i;

    rn = rt_head->root;

    /*
     * Case 0:
     * Attach subtree with prefix 0.0.0.0/0 (or ::/0) to an empty tree.
     * Check whether main tree is empty. If yes, replace the root node with
     * the base node of the subtree and exit.
     */
    if (bitlen == 0) {
        if ((rn->rnode_flags & RNODE_EXTERNAL) ||
                rn->rnode_left || rn->rnode_right) {
            /* main tree is not empty, return */
            return (FALSE);
        }
        rn->rnode_left = st_root->rnode_left;
        rn->rnode_right = st_root->rnode_right;
        /* Replace root with subtree */
        rtn_replace_node(rt_head, rn, st_root);
        return (TRUE);
    }

    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which has info attached.
     */
    while ((rn->rnode_bit < bitlen) || !(rn->rnode_flags & RNODE_EXTERNAL)) {
        if (addr[RNBYTE(rn->rnode_bit)] & RNBIT(rn->rnode_bit)) {
            if (!(rn->rnode_right)) {
                break;
            }
            rn = rn->rnode_right;
        } else {
            if (!(rn->rnode_left)) {
                break;
            }
            rn = rn->rnode_left;
        }
    }

    /*
     * Now we need to find the number of the first bit in our address
     * which differs from his address. Note that rn could be the root
     * without info..
     */
    bits2chk = MIN(rn->rnode_bit, bitlen);
    his_addr = (rn->rnode_flags & RNODE_EXTERNAL) ?
        ((rt_info_t *) rn)->rninfo_key : NULL;

    assert (his_addr || (bits2chk == 0));

    for (dbit = 0; dbit < bits2chk; dbit += RNBBY) {
        i = dbit >> RNSHIFT;
        if (addr[i] != his_addr[i]) {
            dbit += first_bit_set[addr[i] ^ his_addr[i]];
            break;
        }
    }

    /*
     * If the different bit is less than bits2chk we will need to
     * insert a split above him.  Otherwise we will either be in
     * the tree above him, or attached below him.
     */
    if (dbit > bits2chk) {
        dbit = bits2chk;
    }
    rn_prev = rn->rnode_parent;
    while (rn_prev && rn_prev->rnode_bit >= dbit) {
        rn = rn_prev;
        rn_prev = rn->rnode_parent;
    }

    /*
     * Following are the only two cases in which sub-tree will be
     * attached to main tree:
     * 1) st_root added as child of rn inplace of null child node.
     * 2) Splitter in between rnprev and rn. st_root becomes child of
     *    splitter.
     */

    /*
     * If the node rn points at is equal to our bit number then
     * subtree is already part of main tree, just return.
     */
    if (dbit == bitlen && rn->rnode_bit == bitlen) {
        return (FALSE);
    }

    rn_add = st_root;
    /*
     * Check if we can attach directly to the rn.
     */
    if (rn->rnode_bit == dbit) {
        assert(dbit < bitlen);
        /* Case 1
         * Making st_root child of rn
         */
        rn_add->rnode_parent = rn;
        if (addr[RNBYTE(rn->rnode_bit)] & RNBIT(rn->rnode_bit)) {
            assert(!(rn->rnode_right));
            rn->rnode_right = rn_add;
        } else {
            assert(!(rn->rnode_left));
            rn->rnode_left = rn_add;
        }
        return (TRUE);
    }

    /*
     * Check if we have to insert st_root into the tree between
     * rn_prev and rn which implies subtree is already existing.
     */
    if (dbit == bitlen) {
        /* Subtree already there just return */
        return (FALSE);
    } else {
        /*
         * Case 2:
         * We add a split between rn_prev and rn and append the
         * st_root we're adding to one side of it.
         */
        /* Allocating internal splitter node */
        rn_new = rtn_node_alloc(rt_head);
        if (!rn_new) {
            rt_head->ri_count--;
            return (FALSE);
        }
        rn_new->rnode_bit = dbit;

        /* Making rn and st_root childs of splitter node */
        rn_add->rnode_parent = rn_new;
        rn->rnode_parent = rn_new;
        if (addr[RNBYTE(dbit)] & RNBIT(dbit)) {
            rn_new->rnode_right = rn_add;
            rn_new->rnode_left = rn;
        } else {
            rn_new->rnode_left = rn_add;
            rn_new->rnode_right = rn;
        }

        /* Making rn_new (splitter node) child of rn_prev */
        rn_new->rnode_parent = rn_prev;
        if (!rn_prev) {
            ;
        } else if (rn_prev->rnode_right == rn) {
            rn_prev->rnode_right = rn_new;
        } else {
            assert(rn_prev->rnode_left == rn);
            rn_prev->rnode_left = rn_new;
        }
    }

    return (TRUE);
}

/*
 * rtn_get_info_node
 *
 * Return the info node present at addr and bitlen
 */
rt_info_t *
rtn_get_info_node (rt_head_t *rt_head, char *addr, uint16_t bitlen)
{
    rt_node_t *rn;
    int8_t dir_r;

    /*
     * Search down the tree until we find a node which
     * has a bit number the same or greater than ours.
     */
    for (rn = rt_head->root; rn && (rn->rnode_bit < bitlen);) {
        dir_r = rtn_key_nextbit((u_int8_t *) addr, rn->rnode_bit);
        rn = dir_r ? rn->rnode_right : rn->rnode_left;
    }

    return (rtn_node2info(rn));
}
