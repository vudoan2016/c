/** @name Balanced Binary Tree with Prefix Match capabilities.

    This is the generic balanced binary tree that can either act
    as an avl tree, either exogenous or endogenous (embedded within
    your structure for better performance or as standalong nodes
    holding pointers to your structures) and as a prefix matching
    tree that provides best match on masked addresses.

    The API provides a couple
    of macros that you can use to embedd or define the structure of
    appropriate nodes
    and an API operating over such nodes. Start from understanding the
    PRTREE_NODE structure and then move on to the API.

    {\large {\bf The tree operates in host endian mode! }}

    For examples of use, look into
    \URL[prtree_test.c]{http://jay.mtv.siara.com/cgi-bin/perfbrowse.pl?cmd=browse&spec=//depot/.../prtree_test.c}.

    Very simple example of an AVL tree with your own derived node (you could equivalently use
    here a PRTREE_NODE since both have just a key of a single integer but nothing prevents you
    from storing a back pointer to your main structure in mycmp_node and write a comparison function
    that will use this pointer to derive the comparsion from your real structure ebmedding that one.

    \begin{verbatim}

   typedef struct _mycmp_node
    {
      PRTREENODE_EMPTYSTRUCT;
      u_int32_t mykey;
    } mycmp_node;

    int mycmp(PRTREE_HEAD *head,
          void *node1,
          void *node2)
    {
      mycmp_tree *t = (mycmp_head *) head;
      mycmp_node *n1= (mycmp_node *) node1;
      mycmp_node *n2= (mycmp_node *) node2;

      return ( n1->mykey < n2->mykey ? -1 : ( n1->mykey > n2->mykey ? 1 : 0 ));
    }

    void printmykey(void *k)
    {
      mycmp_node *c=(mycmp_node *) k;

      unicli_printf("%d", c->mykey);
    }

    void main(void)
    {
      PRTREE_HEAD headmykey;
      prtree_init(&headmykey,0,0,mycmp,printmykey);
      _new=malloc(sizeof(mycmp_node));
      ((mycmp_node *) _new)->mykey=5;
      prtree_insert_node(&headmykey,
                          _new);
    }

    \end{verbatim}

    @see PRTREE_NODE
    @author Rob Coltun somewhen in the gray past,
            8/99 major rewrite and extending to generic AVL by Tony Przygienda, Ph.D.
*/

//@{

/*
 *
 *
 * prtree.h - Balanced Binary Tree Implementation Header File
 *
 *
 * Copyright (c) 1999-2008 Redback Networks, Inc.
 * Copyright (c) 2010-2016 Ericsson AB.
 * All right reserved.
 *
 * Originating Author: Rob Coltun
 *                     8/99 major rewrite and extending to generic AVL by Tony Przygienda, Ph.D.
 *
 * Warnings/Restrictions:
 *
 * Usage:
 *
 * File abstract: Balanced binary tree header file. This file contains all
 *                the structure and type definitions, constant definitions,
 *                function declarations and macros needed to use the PRTREE
 *                implementation.
 *
 * Open issues:
 *
 */

#ifndef _UTILS_PRTREE_H_
#define _UTILS_PRTREE_H_

#include <sys/types.h>

#define     PRTREE_BIT_SET(F, B)       ((F) |= B)
#define     PRTREE_BIT_RESET(F, B)     ((F) &= ~(B))
#define     PRTREE_BIT_FLIP(F, B)      ((F) ^= (B))
#define     PRTREE_BIT_TEST(F, B)      ((F) & (B))
#define     PRTREE_BIT_MATCH(F, B)     (((F) & (B)) == (B))
#define     PRTREE_BIT_COMPARE(F, B1, B2)  (((F) & (B1)) == B2)
#define     PRTREE_BIT_MASK_MATCH(F, G, B) (!(((F) ^ (G)) & (B)))


#define     prtree_stack_reset(BASE,TOP)   {(TOP) = (BASE);(*TOP) = 0; }
#define     prtree_stack_push(TOP,NODE)    {(*(++(TOP))) = (NODE);}
#define     prtree_stack_pop(TOP)          (*((--(TOP))+1))
#define     prtree_stack_top(TOP)          (*(TOP))
#define     prtree_stack_empty(TOP)        (!(*(TOP)))
#define     prtree_stack_size(BASE,TOP)    ((TOP) - (BASE))

/** @name Structures */
//@{

/* adjust the key size to your usage */
typedef u_int32_t prtree_key_t;

/** generic macro to construct the node of the tree embedded into another
    structure for perfromance reasons, it has no key
    inside. If you need a key of an integer, pick PRTREE_NODE and
    if you need a prefix tree with longest prefix matching, pick
    PRTREE_STRUCT(2). Inside this struct a field, prtree_usr_available
    is 16 bits padding and can be used to store internal values, keys and
    such. Beside that, internal flags can be added for extended functionality
    into prtree_flags.

    @see PRTREE_STRUCT, PRTREE_NODE*/
#define PRTREENODE_EMPTYSTRUCT                          \
   struct _prtree_node  *ptr[2];                        \
   struct _prtree_node  *father;                        \
   struct _prtree_node  *prtree_up;                     \
   u_int8_t              prtree_flags;                  \
   u_int8_t              prtree_balanced;               \
   u_int16_t             prtree_usr_available;          \
   u_int32_t             pad_to_8_bytes;

/** node of a prefix tree, contains a key of one integer. Acts as a generic
    placeholder for a node on the AVL tree so other types of nodes derived by
    the user mostly are casted to that one.

    @see PRTREENODE_EMPTYSTRUCT */
struct _prtree_node
{
  ///
  PRTREENODE_EMPTYSTRUCT;
  /** integer key of the tree */
  prtree_key_t     key[1];
};

/// typedef abbrev
typedef struct _prtree_node prtree_node;
/// typedef abbrev
typedef struct _prtree_node PRTREE_NODE;

/** this macro is used to define a PRTREE node with N keys, use it
    as guts of a typedef'd structure or embedd directly. {\bf All the tree
    routines assume that key[x+1] is more significant than key[x]. }

    @see PRTREENODE_EMPTYSTRUCT */
#define PRTREE_STRUCT(N)       \
    PRTREENODE_EMPTYSTRUCT;    \
    prtree_key_t   key[N]

/*
 * Head of prefix tree. Never access directly but through initialization API
 *
 *   ptr    - keep 3 pointers in here to pretend we have a right, left and
 *            father because otherwise father adjustement fails since head is
 *            sometimes casted to node
 *   keylen - specially encoded field, allows to check what the length of the
 *            key is in the tree and whether a special comparison function
 *            has been initialized
 *   radix  - indicates whether tree is radix
 *   nodes  - counter of nodes in the tree
 *
 */
#define PRTREEHEAD_EMPTYSTRUCT                          \
   struct _prtree_node  *ptr[3];                        \
   int                   keylen;                        \
   int                   radix;                         \
   int                   nodes;


/* used as the structure for defining a prtree head */
struct _prtree_head
{
    PRTREEHEAD_EMPTYSTRUCT;

    /** comparision function if keylen==0 */
    int    (*cmp)(struct _prtree_head *head, void *node1, void *node2);
    /** print out key function if keylen==0 */
    void   (*keyout)(void *);
};

/* used as the structure for defining a minimum prtree head for keylen != 0 */
struct _prtree_head_min
{
    PRTREEHEAD_EMPTYSTRUCT;
};

/// typedef abbrev
typedef struct _prtree_head      prtree_head;
typedef struct _prtree_head      PRTREE_HEAD;
typedef struct _prtree_head_min  prtree_head_min;
typedef struct _prtree_head_min  PRTREE_HEAD_MIN;

typedef int prtree_bool;
typedef int (*prtree_cmp)(PRTREE_HEAD *, void *, void *);

#define PRTREE_RLINK            0
#define PRTREE_LLINK            1
#define PRTREE_BALANCED         2
#define PRTREE_EMPTY            3
#define PRTREE_EQUAL            PRTREE_BALANCED
#define PRTREE_OPP(B)           (((B) == PRTREE_LLINK) ? PRTREE_RLINK : PRTREE_LLINK)
#define MAX_PRTREE_STACK_SIZE   50

#define PRTREE_CMP_BITS(T)              (1 << (T))
#define PRTREE_WHICH_LINK(CUR,SON)\
         ((CUR)->ptr[PRTREE_RLINK] == (SON)) ? PRTREE_RLINK : PRTREE_LLINK

#define PRTREE_FLAGS_DELETE     0x01

/// if using a 2 key tree as prefix tree, this macro indexes address key part
#define PRTREE_KEY_ADDR         1
/// if using a 2 key tree as prefix tree, this macro indexes mask key part
#define PRTREE_KEY_MASK         0

#define PRTREE_1KEY_LEN         1
#define PRTREE_2KEY_LEN         2

#define PRTREENULL ((prtree_node *)0)

/// arguments to prtree_delete_node_shareable()
#define PRTREE_DEL_ANY_MATCH           0
#define PRTREE_DEL_SPECIFIED_NODE      1

#define PRTREE_KeyCmp(RET,LEN,KEY1,KEY2)\
    {\
        prtree_key_t *_kp1 = &KEY1[LEN], *_kp2 = &KEY2[LEN];\
        int _len = (LEN);\
        while(_len--)\
        {\
            _kp1--;_kp2--;\
            if ((*_kp1) < (*_kp2))\
            {\
                (RET) = PRTREE_LLINK;\
                break;\
            }\
            else if ((*_kp1) > (*_kp2))\
            {\
                (RET) = PRTREE_RLINK;\
                break;\
            }\
            else\
            {\
                (RET) = PRTREE_EQUAL;\
            }\
        }\
    }

//@}

/** @name API */
//@{

#ifdef __cplusplus
extern "C"
{
#endif


/** @name Tree initialization */
//@{

/** initializes a head of the tree.

    @param PRTREE_HEAD* pointer to head of tree
    @param keylen  length of the key, if the node structure embeds one integer (PRTREE_NODE)
                   indicate it by passing 1, for a tree holding keys generated by using
                   PRTREE_STRUCT(N) pass N in, 0 is a special case and indicates that the
                   key is not embedded within the node but the passed in comparison function
                   should be used that is passed as another argument to compare elements of the
                   tree.
    @param radix   indicates whether the tree is a radix tree. Makes only sense if keylen is 2.
                   If keylen is not 2, tree is automatically {\bf not} a radix tree.
    @param cmp     comparison function, equivalent to strcmp for two nodes being passed in instead
                   of strings. Only of interest if keylen is 0
    @param keyout  optional funciton that prints the key value to stderr when the tree keylen is 0

    @see PRTREE_NODE, PRTREE_STRUCT */
static inline void prtree_init(PRTREE_HEAD  *head,
                   int          keylen,
                   prtree_bool  radix,
                   prtree_cmp   cmp,
                   void         (*keyout)(void *))
{
  head->ptr[PRTREE_RLINK] = PRTREENULL;
  head->keylen            = keylen;
  head->cmp               = cmp;
  head->radix             = (radix && keylen==2 ? radix : 0);
  head->keyout            = keyout;
  head->nodes             = 0;
}

static inline void prtree_init_min(PRTREE_HEAD_MIN   *head,
                   int               keylen,
                   prtree_bool       radix)
{
  head->ptr[PRTREE_RLINK] = PRTREENULL;
  head->keylen            = keylen;
  head->radix             = (radix && keylen==2 ? radix : 0);
  head->nodes             = 0;
}

/** initializes a prefix tree, pretty much jsut an abbreviation for prtree_init with key length 2,
    radix set and no special routines to compare or print keys.

    @see prtree_init */
static inline void prtree_init_prefix(PRTREE_HEAD *head)
{
  prtree_init(head, 2, 1,
              (prtree_cmp) 0,
              (void (*)(void *)) 0);
}

/** frees the tree, all elements are removed, be aware that the tree is not being walked
    and therefore all the elements are left dangling.

    @param PRTREE_HEAD* pointer to head of tree */
static inline void prtree_free(PRTREE_HEAD *head)
{
  head->ptr[PRTREE_RLINK] = PRTREENULL;
  head->keylen=0;
  head->cmp = (prtree_cmp) 0;
}

//@} // of inits

/** @name Matching elements in the tree */
//@{

/** matches a node on tree or if key not present, smallest of the bigger ones. Returns deleted nodes as well.

    @param head of the tree, must be initialized of course
    @param keyholder is appropriate node pointer filled in with the key to be found
    @param cmp_func is a comparison function that overrides the one the tree was initialized with
    @return PRTREENULL if not found or any error, pointer to the node or next bigger otherwise
*/
prtree_node *prtree_exact_or_larger_match(prtree_head *head,
                                         prtree_node *keyholder );
prtree_node *prtree_exact_or_larger_match_shareable(prtree_head *head,
                                                    prtree_node *keyholder,
                                                    prtree_cmp cmp_func );

/** matches a node on tree the node with the next largest key. Returns deleted nodes as well.

    @param head of the tree, must be initialized of course
    @param keyholder is appropriate node pointer filled in with the key to be found
    @param cmp_func is a comparison function that overrides the one the tree was initialized with
    @return PRTREENULL if not found or any error, pointer to the next bigger node
*/
prtree_node *prtree_larger_match(prtree_head *head,
                                 prtree_node *keyholder);
prtree_node *prtree_larger_match_shareable(prtree_head *head,
                                           prtree_node *keyholder,
                                           prtree_cmp cmp_func);

/** matches a node on tree or if key not present, largest of the smaller ones. Returns deleted nodes as well.

    @param head of the tree, must be initialized of course
    @param keyholder is appropriate node pointer filled in with the key to be found
    @param cmp_func is a comparison function that overrides the one the tree was initialized with
    @return PRTREENULL if not found or any error, pointer to the node or last smaller otherwise
*/
prtree_node *prtree_exact_or_smaller_match(prtree_head *head,
                                          prtree_node *keyholder );
prtree_node *prtree_exact_or_smaller_match_shareable(prtree_head *head,
                                                     prtree_node *keyholder,
                                                     prtree_cmp cmp_func );

/** matches a node on tree with the next smaller key . Returns deleted nodes as well.

    @param head of the tree, must be initialized of course
    @param keyholder is appropriate node pointer filled in with the key to be found
    @param cmp_func is a comparison function that overrides the one the tree was initialized with
    @return PRTREENULL if not found or any error, pointer to the node or last smaller otherwise
*/
prtree_node *
prtree_smaller_match (prtree_head *head_p, prtree_node *keyholder_p);
prtree_node *
prtree_smaller_match_shareable (prtree_head *head_p, prtree_node *keyholder_p,
                                prtree_cmp cmp_func);
/** matches exactly on a tree.

    @param head of the tree, must be initialized of course
    @param keyholder is appropriate node pointer filled in with the key to be found
    @param return_deleted determines whether nodes marked as deleted should be considered matches or not
    @param cmp_func is a comparison function that overrides the one the tree was initialized with
    @return PRTREENULL if not found or any error, pointer to the node otherwise
*/
prtree_node * prtree_exact_match(prtree_head *head, PRTREE_NODE *keyholder, prtree_bool return_deleted);
prtree_node * prtree_exact_match_shareable(prtree_head *head, PRTREE_NODE *keyholder,
                                           prtree_bool return_deleted,
                                           prtree_cmp cmp_func);

/** Best match for tree with address and mask keys. Tree must be radix.

   @param key is the address to be matched
   @return best match (the one with longest mask) whereas key[PRTREE_KEY_ADDR] is masked by key[PRTREE_KEY_MASK]
           or NULL if error, tree not radix or no match.
*/
prtree_node * prtree_2key_best_match(prtree_head *head, prtree_key_t key, prtree_bool return_deleted );


/** For a node that matched, this routine delivers a match with a shorter mask. That way all matches, from the most
    specific until the least specific one can be walked. Tree must be radix.

   @param _last_match is the node that matched last time and for which the next less specific match should be returned
   @param return_deleted  should return deleted nodes as well ?
   @return next best match (the one with next shorter mask) whereas key[PRTREE_KEY_ADDR] is masked by key[PRTREE_KEY_MASK]
           or NULL if error, tree not radix or no match.
*/
prtree_node * prtree_2key_best_match_get_next_match(prtree_head *head,  prtree_node *_last_match, prtree_bool return_deleted );


/** Lookup up a generic tree using a subset of the key
    return only the first matching node

    @param head of the tree, must be initialized of course
    @param key_len the length of the key (in u_int32_t)
    @param keyholder is appropriate node pointer filled in with the key to be found
    @param return_deleted determines whether nodes marked as deleted should be considered matches or not
    @return PRTREENULL if not found or any error, pointer to the node otherwise
*/
prtree_node *
prtree_partial_match (prtree_head *head,
              int key_len,
              PRTREE_NODE *keyholder,
              prtree_bool return_deleted);

//@}   // of matching




/** @name Adding and deleting elements */
//@{
/** insert a new node into the tree. If tree is radix, PRTREE_KEY_ADDR and _MASK must be used
    to store the appropriate values into keys. {\bf Observe that in case of radix trees, PRTREE_KEY_ADDR
    is masked with _MASK key before being inserted into the tree}.

    @param head of the tree
    @param new is the new node with key filled in
    @param cmp_func is a comparison function that overrides the one the tree was initialized with
    @return PRTREENULL if failed or returns duplicate node if present
*/
prtree_node * prtree_insert_node(prtree_head *head, prtree_node *_new );
prtree_node * prtree_insert_node_shareable(prtree_head *head, prtree_node *_new,
                                           prtree_cmp cmp_func );

/** deletes a node from the tree. It rebalances the tree. If you're temporarily deleting and then reintroducting lots of nodes
    on a tree, alternatively the delete flag can be set/reset and only the {\em really to be deleted ;-)} elements deleted at
    the end.

    @param head of the tree
    @param del_node is node to be deleted. Its validity is checked.
    @param cmp_func is a comparison function that overrides the one the tree was initialized with
    @return PRTREENULL if failed or duplicate present
*/
prtree_node * prtree_delete_node(prtree_head *head, PRTREE_NODE *del_node);
prtree_node * prtree_delete_specified_node(prtree_head *head, PRTREE_NODE *del_node);
prtree_node * prtree_delete_node_shareable(prtree_head *head, PRTREE_NODE *del_node,
                                           prtree_cmp cmp_func,
                                           prtree_bool only_specified_node);
//@}

/** @name Navigating around the tree */
//@{

/** returns first element in the tree. {\bf slow!}

    @param head of the tree
    @param return_deleted determines whether nodes marked as deleted should be considered matches or not
    @return first element or NULL if tree empty or error
*/
prtree_node * prtree_first(prtree_head *head, prtree_bool return_deleted);

/** returns last element in the tree. {\bf slow!}

    @param head of the tree
    @param return_deleted determines whether nodes marked as deleted should be considered matches or not
    @return last element or NULL if tree empty or error
*/
prtree_node * prtree_last(prtree_head *head, prtree_bool return_deleted);

/** returns next element in the tree. Should be used in walks only if delete operations during the walk can
    occur, preferrably the walk macros should be
    used since this routine is somehow slower. It alwasy returns deleted elements.

    @param head of the tree
    @param node is the previous node to return the element after, _must_ be on tree
    @param return_deleted determines whether nodes marked as deleted should be considered matches or not
    @return first element or NULL if tree empty or error

    @see PRTREE_2KEY_WALK_NEXT_BEGIN, PRTREE_1KEY_WALK_NEXT_BEGIN
*/
prtree_node * prtree_next(prtree_head *head, PRTREE_NODE *node);

/** returns previous element in the tree. Should be used in walks only if delete operations during the walk can
    occur, preferrably the walk macros should be
    used since this routine is somehow slower. It alwasy returns deleted elements.

    @param head of the tree
    @param node is the next larger node to return element
    @return first element or NULL if tree empty or error
*/
prtree_node * prtree_prev(prtree_head *head, PRTREE_NODE *node);

/** Walk the tree. Defined as macro for performance. Must be framed by
   PRTREE_WALK_END. {\bf Caution: you can{\em not} delete elements
   during this walk, use prtree_first & prtree_next loops for that}.

    @param S is the stack to be used, you want something of {\em prtree_node *stack[MAX_DEPTH] size} type
    @param H head of the tree, prtree_head *
    @param B is prtree_node * or pointer to your self defined type of prtree node by using the appropriate macros

    @see PRTREE_WALK_END */
#define PRTREE_WALK_BEGIN(S,H,B)\
{ prtree_node **_top; prtree_stack_reset((S),_top);\
    (B) = (H)->ptr[PRTREE_RLINK];\
    if (B) {\
        while(1) {\
            while ((B)) {prtree_stack_push(_top,(B));(B) = (B)->ptr[PRTREE_LLINK];}\
            if (!((B) = prtree_stack_pop(_top))) break;\
            if (!PRTREE_BIT_TEST((B)->prtree_flags,PRTREE_FLAGS_DELETE)) {\

/** Macro to frame the walk the tree at the end. Defined as macro for performance.

    @see PRTREE_WALK_BEGIN
*/
#define PRTREE_WALK_END(B)\
            } if (((B) = (B)->ptr[PRTREE_RLINK]) != PRTREENULL)\
              { prtree_stack_push(_top,(B)); (B) = (B)->ptr[PRTREE_LLINK];}\
        }\
    }\
}

/** breaks out the PRTREE_WALK_BEGIN macro

    @see PRTREE_WALK_BEGIN */
#define PRTREE_WALK_BREAK break

/** Walk from addr to end of tree. Must be framed by
   PRTREE_1KEY_WALK_NEXT_END. {\bf Caution: you can{\em not} delete
   elements during this walk, use prtree_first & prtree_next loops for
   that}.

    @param S is the stack to be used, you want something of {\em prtree_node *stack[MAX_DEPTH] size} type
    @param H head of the tree, prtree_head *
    @param B is prtree_node * or pointer to your self defined type of prtree node by using the appropriate macros
    @param K is the key  to start from
    @param label_string is a temporary label used internally to break the loop */
#define PRTREE_1KEY_WALK_NEXT_BEGIN(S,H,B,K,label_string)\
{\
    prtree_node **__top; int __cmp; prtree_key_t __key;\
    __key = (K);\
    prtree_stack_reset((S),__top);\
    (B) = (H)->ptr[PRTREE_RLINK];\
    if (!B) goto __finish_up1_##label_string;\
    while(1)\
    {\
        __cmp = (__key < (B)->key[0]) ? PRTREE_LLINK : (__key > (B)->key[0]) ? PRTREE_RLINK : PRTREE_EQUAL;\
        if (__cmp == PRTREE_EQUAL) break;\
        if (__cmp == PRTREE_LLINK)\
        {\
            prtree_stack_push(__top,(B));\
        }\
        if (!((B) = (B)->ptr[__cmp]))\
        {\
            break;\
        }\
    }\
    while(1)\
    {\
        while (B)\
        {\
            prtree_stack_push(__top,(B));\
            (B) = (B)->ptr[PRTREE_LLINK];\
        }\
        if (!((B) = prtree_stack_pop(__top)))\
        {\
            goto __finish_up1_##label_string;\
        }\
        __cmp = (__key < (B)->key[0]) ? PRTREE_LLINK : (__key > (B)->key[0]) ? PRTREE_RLINK : PRTREE_EQUAL;\
        if (__cmp != PRTREE_RLINK)\
        {\


/** frames PRTREE_1KEY_WALK_NEXT_BEGIN macro

   @see PRTREE_1KEY_WALK_NEXT_BEGIN
*/
#define PRTREE_1KEY_WALK_NEXT_END(B,label_string)\
        }\
        if ( ((B) = (B)->ptr[PRTREE_RLINK]) != PRTREENULL )\
        {\
            prtree_stack_push(__top,(B));\
            (B) = (B)->ptr[PRTREE_LLINK];\
        }\
    }\
    __finish_up1_##label_string:\
    (B) = PRTREENULL;\
}

/** analog to PRTREE_1KEY_WALK_NEXT_BEGIN but for trees with 2 integers as key, acting as prefix
   tree (keys are interpreted as mask and address). {\bf Caution: you can{\em not} delete
   elements during this walk, use prtree_first & prtree_next loops for that}.

   @see PRTREE_1KEY_WALK_NEXT_BEGIN */
#define PRTREE_2KEY_WALK_NEXT_BEGIN(S,H,B,K,label_string)\
{\
    prtree_node **__top; int __cmp;\
    prtree_stack_reset((S),__top);\
    (B) = (H)->ptr[PRTREE_RLINK];\
    if (!B) goto __finish_up2_##label_string;\
    while(1)\
    {\
        __cmp = ((K[1]) < (B)->key[1]) ? PRTREE_LLINK :\
                ((K[1]) > (B)->key[1]) ? PRTREE_RLINK :\
                        ((K[0]) < (B)->key[0]) ? PRTREE_LLINK :\
                            ((K[0]) > (B)->key[0]) ? PRTREE_RLINK : PRTREE_EQUAL;\
        if (__cmp == PRTREE_EQUAL) break;\
        if (__cmp == PRTREE_LLINK)\
        {\
            prtree_stack_push(__top,(B));\
        }\
        if (!((B) = (B)->ptr[__cmp]))\
        {\
            break;\
        }\
    }\
    while(1)\
    {\
        while (B)\
        {\
            prtree_stack_push(__top,(B));\
            (B) = (B)->ptr[PRTREE_LLINK];\
        }\
        if (!((B) = prtree_stack_pop(__top)))\
        {\
            goto __finish_up2_##label_string;\
        }\
        __cmp = ((K[1]) < (B)->key[1]) ? PRTREE_LLINK :\
                ((K[1]) > (B)->key[1]) ? PRTREE_RLINK :\
                        ((K[0]) < (B)->key[0]) ? PRTREE_LLINK :\
                            ((K[0]) > (B)->key[0]) ? PRTREE_RLINK : PRTREE_EQUAL;\
        if (__cmp != PRTREE_RLINK)\
        {\


/** frames PRTREE_2KEY_WALK_NEXT_BEGIN.

    @see PRTREE_2KEY_WALK_NEXT_BEGIN
*/
#define PRTREE_2KEY_WALK_NEXT_END(B, label_string)\
        }\
        if (((B) = (B)->ptr[PRTREE_RLINK]) != PRTREENULL)\
        {\
            prtree_stack_push(__top,(B));\
            (B) = (B)->ptr[PRTREE_LLINK];\
        }\
    }\
    __finish_up2_##label_string:\
    (B) = PRTREENULL;\
}

/** Walk in reverse order from addr to beginning of tree.
   {\bf Caution: you can{\em not} delete
   elements during this walk, use prtree_first & prtree_next loops for
   that}.

    @param S is the stack to be used, you want something of {\em prtree_node *stack[MAX_DEPTH] size} type
    @param H head of the tree, prtree_head *
    @param B is prtree_node * or pointer to your self defined type of prtree node by using the appropriate macros
    @param K is the key  to start from
    @param label_string is a temporary label used internally to break the loop */
#define PRTREE_2KEY_WALK_PREV_BEGIN(S,H,B,K,label_string)\
{\
    prtree_node **__top; int __cmp;\
    prtree_stack_reset((S),__top);\
    (B) = (H)->ptr[PRTREE_RLINK];\
    if (!B) goto __finish_up2_##label_string;\
    while(1)\
    {\
        __cmp = ((K[1]) < (B)->key[1]) ? PRTREE_LLINK :\
                ((K[1]) > (B)->key[1]) ? PRTREE_RLINK :\
                        ((K[0]) < (B)->key[0]) ? PRTREE_LLINK :\
                            ((K[0]) > (B)->key[0]) ? PRTREE_RLINK : PRTREE_EQUAL;\
        if (__cmp == PRTREE_EQUAL) break;\
        if (__cmp == PRTREE_RLINK)\
        {\
            prtree_stack_push(__top,(B));\
        }\
        if (!((B) = (B)->ptr[__cmp]))\
        {\
            break;\
        }\
    }\
    while(1)\
    {\
        while (B)\
        {\
            prtree_stack_push(__top,(B));\
            (B) = (B)->ptr[PRTREE_RLINK];\
        }\
        if (!((B) = prtree_stack_pop(__top)))\
        {\
            goto __finish_up2_##label_string;\
        }\
        __cmp = ((K[1]) < (B)->key[1]) ? PRTREE_LLINK :\
                ((K[1]) > (B)->key[1]) ? PRTREE_RLINK :\
                        ((K[0]) < (B)->key[0]) ? PRTREE_LLINK :\
                            ((K[0]) > (B)->key[0]) ? PRTREE_RLINK : PRTREE_EQUAL;\
        if (__cmp != PRTREE_LLINK)\
        {\


/** frames PRTREE_2KEY_WALK_PREV_BEGIN.

    @see PRTREE_2KEY_WALK_PREV_BEGIN
*/
#define PRTREE_2KEY_WALK_PREV_END(B, label_string)\
        }\
        if (((B) = (B)->ptr[PRTREE_LLINK]) != PRTREENULL)\
        {\
            prtree_stack_push(__top,(B));\
            (B) = (B)->ptr[PRTREE_RLINK];\
        }\
    }\
    __finish_up2_##label_string:\
    (B) = PRTREENULL;\
}

/** PRTREE_NKEY_WALK_NEXT_BEGIN:
   analog to PRTREE_1KEY_WALK_NEXT_BEGIN but for trees with arbituray key and
   a COMPARE function, acting as prefix tree (keys are interpreted as mask and
   address). {\bf Caution: you can{\em not} delete elements during this walk,
   use prtree_first & prtree_next loops for that}.
   S - Stack
   H - Root of the tree
   B - Current Node
   K - Key Node to begin with
   label_string - arbituary string used as part of label

   @see PRTREE_1KEY_WALK_NEXT_BEGIN */

#define PRTREE_NKEY_WALK_NEXT_BEGIN(S,H,B,K,label_string) \
{ \
    prtree_node **__top; \
    int __cmp = PRTREE_EQUAL; \
    prtree_stack_reset((S),__top); \
    (B) = (H)->ptr[PRTREE_RLINK]; \
    if (!B) goto __finish_up2_##label_string; \
    while(1) { \
        if ((H)->keylen > 0) { \
            PRTREE_KeyCmp(__cmp, (H)->keylen, (K)->key, (B)->key); \
        } else { \
            __cmp = (H)->cmp(H, K, B); \
            __cmp = ( __cmp < 0 ? PRTREE_LLINK : \
                    ( __cmp > 0 ? PRTREE_RLINK : PRTREE_EQUAL )); \
        } \
        if (__cmp == PRTREE_EQUAL) break; \
        if (__cmp == PRTREE_LLINK) { \
            prtree_stack_push(__top,(B)); \
        } \
        if (!((B) = (B)->ptr[__cmp])) {\
            break; \
        } \
    } \
    while(1) { \
        while (B) { \
            prtree_stack_push(__top,(B)); \
            (B) = (B)->ptr[PRTREE_LLINK]; \
        } \
        if (!((B) = prtree_stack_pop(__top))) { \
            goto __finish_up2_##label_string; \
        } \
        if ((H)->keylen > 0) { \
            PRTREE_KeyCmp(__cmp, (H)->keylen, (K)->key, (B)->key); \
        } else { \
            __cmp = (H)->cmp(H, K, B); \
            __cmp = ( __cmp < 0 ? PRTREE_LLINK : \
                    ( __cmp > 0 ? PRTREE_RLINK : PRTREE_EQUAL ));   \
        } \
        if (__cmp != PRTREE_RLINK) { \


/** PRTREE_NKEY_WALK_NEXT_END.

    @see PRTREE_NKEY_WALK_NEXT_BEGIN
*/
#define PRTREE_NKEY_WALK_NEXT_END(B, label_string) \
        } \
        if (((B) = (B)->ptr[PRTREE_RLINK]) != PRTREENULL) { \
            prtree_stack_push(__top,(B)); \
            (B) = (B)->ptr[PRTREE_LLINK]; \
        } \
    } \
    __finish_up2_##label_string: \
    (B) = PRTREENULL; \
}


/** Walk all prtree entries within the given address range. {\bf
   Caution: you can{\em not} delete elements during this walk, use
   prtree_first & prtree_next loops for that or the delete flag
   and walk the tree deleting the delete marked nodes then.}.

    @param S is the stack to be used, you want something of {\em prtree_node *stack[MAX_DEPTH] size} type
    @param H head of the tree, prtree_head *
    @param B is prtree_node * or pointer to your self defined type of prtree node by using the appropriate macros
    @param addr is the address that when ended with mask indicates where to start from, without, where to stop
    @param mask is the mask to start from */
#define PRTREE_RANGE_WALK_BEGIN(S,H,B,ADDR,MASK)\
{\
        int     _cmp, _over = 0;\
        prtree_node *_a, **_ipr_top;\
        u_int32_t _last_addr = (ADDR) | ~(MASK);\
        _a = (H)->ptr[PRTREE_RLINK];\
        if (_a) \
        {\
            prtree_stack_reset(S,_ipr_top);\
            while(1)\
            {\
                if ((_a->key[1] & (MASK)) > (ADDR))\
                {\
                    _cmp = PRTREE_LLINK;\
                }\
                else if ((_a->key[1] & (MASK)) < (ADDR))\
                {\
                    _cmp = PRTREE_RLINK;\
                }\
                else\
                {\
                    while(1)\
                    {\
                        while (_a)\
                        {\
                            if ((_a->key[1] & (MASK)) < (ADDR))\
                            {\
                                if (_over) break;\
                                else _over++;\
                            }\
                            prtree_stack_push(_ipr_top,_a);\
                            _a = _a->ptr[PRTREE_LLINK];\
                        }\
                        _over = 0;\
                        if (!(_a = prtree_stack_pop(_ipr_top))) break;\
                        if ((_a->key[1] & (MASK)) == (ADDR) && \
                            (_a->key[0]) >= (MASK))\
                        {\
                           (B) = _a;

/** frames PRTREE_RANGE_WALK_BEGIN */
#define PRTREE_RANGE_WALK_END \
                        }\
                        if ((_a = _a->ptr[PRTREE_RLINK]) != PRTREENULL)\
                        {\
                            if (_a->key[1] <= _last_addr)\
                            {\
                                prtree_stack_push(_ipr_top,_a);\
                            }\
                            _a = _a->ptr[PRTREE_LLINK];\
                        }\
                    }\
                    break;\
                }\
                if (!_a || !(_a = _a->ptr[_cmp])) break;\
            }\
        }\
}

/** Walk all prtree entries that match a given prefix (or) partial
    key within the given prtree. {\bf Caution: you can{\em not}
    delete elements during this walk, use the delete flag
    and walk the tree deleting the delete marked nodes then.}.
    S - Stack
    H - Root of the tree
    B - Current Node
    K - Key Node to begin with
    label_string - arbituary string used as part of label

    Node - Current node being compared (__cmp == ?)
    LC - Left Child
    RC - Right Child

       |
     Node
     /  \
    LC  RC

    __cmp == PRTREE_EQUAL?   Walk both LC & RC, if LC & RC exist
    __cmp == PRTREE_LLINK ?   Walk LC, if LC exists
    __cmp == PRTREE_RLINK ?   Walk RC, if RC exists
**/
#define PRTREE_PREFIX_MATCH_WALK_NEXT_BEGIN(S,H,B,K,label_string) \
{ \
    prtree_node **__top; \
    int __cmp = PRTREE_EQUAL; \
    prtree_stack_reset((S),__top); \
    (B) = (H)->ptr[PRTREE_RLINK]; \
    if (!B) goto __finish_up2_##label_string; \
    while(1) { \
        if ((H)->keylen > 0) { \
            PRTREE_KeyCmp(__cmp, (H)->keylen, (K)->key, (B)->key); \
        } else { \
            __cmp = (H)->cmp(H, K, B); \
            __cmp = ( __cmp < 0 ? PRTREE_LLINK : \
                    ( __cmp > 0 ? PRTREE_RLINK : PRTREE_EQUAL )); \
        } \
        if (__cmp == PRTREE_EQUAL) { \
            prtree_stack_push(__top,(B)); \
            break;\
        } \
        if (!((B) = (B)->ptr[__cmp])) {\
            break; \
        } \
    } \
    while(1) { \
        if (!((B) = prtree_stack_pop(__top))) { \
            goto __finish_up2_##label_string; \
        } \
        if ((H)->keylen > 0) { \
            PRTREE_KeyCmp(__cmp, (H)->keylen, (K)->key, (B)->key); \
        } else { \
            __cmp = (H)->cmp(H, K, B); \
            __cmp = ( __cmp < 0 ? PRTREE_LLINK : \
                    ( __cmp > 0 ? PRTREE_RLINK : PRTREE_EQUAL )); \
        } \
        if (__cmp == PRTREE_EQUAL) { \

/** Macro to be used with PRTREE_PREFIX_MATCH_WALK_NEXT_BEGIN.
    Cannot be used separately **/
#define PRTREE_PREFIX_MATCH_WALK_NEXT_END(H,B,K,label_string) \
            if ((B)->ptr[PRTREE_LLINK]) {\
                prtree_stack_push(__top,(B)->ptr[PRTREE_LLINK]); \
            }\
            if ((B)->ptr[PRTREE_RLINK]) {\
                prtree_stack_push(__top,(B)->ptr[PRTREE_RLINK]); \
            }\
        } else {\
            (B) = (B)->ptr[__cmp]; \
            if ((B)) {\
                prtree_stack_push(__top,(B)); \
            }\
        }\
    } \
    __finish_up2_##label_string: \
    (B) = PRTREENULL; \
}


//@}  // of navigation api

/** @name Macros for quick deleting/undeleting nodes without
 removing/adding them to the tree.

 Usefull in walks where you cannot
 delete. */
//@{

/**  Set an entry as being deleted. Doesn't remove it from tree and allows for quick marking/unmarking.
     To effectively remove from tree use prtree_delete_node.

     @see  prtree_delete_node */
#define PRTREE_SET_DELETE(B)\
        PRTREE_BIT_SET((B)->prtree_flags,PRTREE_FLAGS_DELETE);

/**  Restore quickly a deleted entry. Assumes that node is already on the tree.

     @see  prtree_delete_node */
#define PRTREE_UNSET_DELETE(B)\
        PRTREE_BIT_RESET((B)->prtree_flags,PRTREE_FLAGS_DELETE);

/** quickly tests whether a node has been marked as deleted or not */
#define PRTREE_TEST_DELETE(B)\
        PRTREE_BIT_TEST((B)->prtree_flags,PRTREE_FLAGS_DELETE)

//@} of delete/undelete macros


/** Place all entries in a single linked list off the tree head.
    Calling routine is then responsible for freeing resources.

    Obscure and not understood. Rob should fill in here ...
*/
void prtree_queue_nodes(prtree_head *head );


/** returns number of nodes on the tree (counting deleted). Very fast.

    @param head of the tree
    @return number of nodes in tree including marked deleted */
static inline int prtree_nodes(prtree_head *head)
{
  if (!head) return 0;
  return head->nodes;
}

/** prints a tree to stdout. For debugging only */
void prtree_print(prtree_head *head);

#ifdef __cplusplus
};
#endif

#endif /* _UTILS_PRTREE_H_ */

//@}  // of API

//@}
