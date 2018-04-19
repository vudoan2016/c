/*
 * xtimers.c
 *
 * Copyright (c) 2015 - 2016 Ericsson AB.
 * All rights reserved.
 *
 * The xtimer is a tree-based timer facility derived from the ptimer
 * but is simpler and scales better due to its use of the absolute
 * system timestamp, and a balanced binary tree for the timers.
 */

#include "corelibs/rbtree.h"
#include "corelibs/chunk.h"
#include "corelibs/xtimers.h"

/*
 * Flags for a timer.
 */
#define XTIMER_FLAG_EXPIRED      0x01      /* timer in the expired queue */
#define XTIMER_FLAG_RUNNING      0x02      /* timer in the tree, running */

/*
 * Timers with an identical expiration time are stored in a linklist
 * off a tree node. The tree node is keyed on the expiration time
 * which is the system time in microseconds (usec).
 */
typedef struct xtimer_bucket
{
    rbnode_t           stent;                /* rbtree node */
    u_int64_t          tm_expire;            /* expire time */
    dbl_qhead_t        timerQ;               /* chained timers */
} xtimer_bucket_t;

typedef struct xtimer_pool
{
    /*
     * Global variables.
     */
    dbl_qhead_t     xtimer_expiredQ;      /* all the expired timers */
    rbtree_t        xtimer_tree;          /* tree for running timers */
    chunk_header_t  *xtimer_chunk;        /* for tree node */
    u_int32_t       xtimer_unit;          /* timer resolution */
    u_int32_t       xtimer_bits;          /* bits to be shifted */

    pthread_mutex_t xtimer_w_mutex;       /* for the timer thread */
    pthread_cond_t  xtimer_w_cond;        /* for the timer thread */
    u_int64_t       xtimer_w_expire;      /* expiration time for timer thread */
    clockid_t       xtimer_clock_type;    /* xtimer clock type */

    /*
     * Stats for timers.
     */
    int             xtimer_curr_running;  /* total # of running timers */
    int             xtimer_max_running;   /* max # of running timers */
    int             xtimer_max_expired;   /* max # of expired timers */
} xtimer_pool_t;

static xtimer_pool_t xtimer_default_pool;
void* xtimer_glob_pool = (void*) &xtimer_default_pool;
const xtimer_flags_t default_xtimer_flags = XTIMER_FLAGS_NONE;

/*
 * xtimer_mutex_lock
 *
 * Locks the xtimer mutex.
 */
static inline void
xtimer_mutex_lock (pthread_mutex_t *mutex)
{
    pthread_mutex_lock(mutex);
}

/*
 * xtimer_mutex_unlock
 *
 * Un-locks the xtimer mutex.
 */
static inline void
xtimer_mutex_unlock (pthread_mutex_t *mutex)
{
    pthread_mutex_unlock(mutex);
}

/*
 * xtimer_nptlonly_mutex_lock
 *
 * Locks the xtimer mutex only if running in NPTL mode.
 */
static inline void
xtimer_nptlonly_mutex_lock (pthread_mutex_t *mutex)
{
    pthread_nptlonly_mutex_lock(mutex);
}

/*
 * xtimer_nptlonly_mutex_unlock
 *
 * Un-locks the xtimer mutex only if running in NPTL mode.
 */
static inline void
xtimer_nptlonly_mutex_unlock (pthread_mutex_t *mutex)
{
    pthread_nptlonly_mutex_unlock(mutex);
}

/*
 * xtimer_node_free_func
 *
 * node free function for the xtimer tree.
 */
static void
xtimer_node_free_func (rbnode_t *bucket, void *ctx)
{
    xtimer_pool_t* xtp = (xtimer_pool_t*) ctx;
    if (xtp != NULL) {
        chunk_free(xtp->xtimer_chunk, bucket);
    }
}

/*
 * find_max_bits
 *
 * Get the integer portion of log(n).
 */
static int
find_max_bits (u_int32_t n)
{
    int  i;

    for (i = 0; n != 0; i++) {
        n >>= 1;
    }
    return (i-1);
}

static inline clockid_t
xtimer_get_tstamp_clock_type (void)
{
    return (CLOCK_MONOTONIC);
}

static inline u_int64_t
xtimer_tstamp_us (const xtimer_pool_t* xtp)
{
    u_int64_t usecs = 0;
    if (rbn_tstamp_us_with_clocktype(xtp->xtimer_clock_type, &usecs) != 0) {
        assert(((void)"rbn_tstamp_us_with_clocktype failed", 0));
    }
    return (usecs);
}

static int
xtimer_pool_init_internal (xtimer_pool_t* xtp,
                           u_int32_t time_unit_ms,
                           xtimer_clock_type_t clock_type)
{
    int32_t ret;
    pthread_condattr_t xtimer_w_condattr;

    if (xtp == NULL) {
        return (-EINVAL);
    }
    else if (time_unit_ms == 0) {
        xtp->xtimer_unit = 50 * MSEC_TO_USEC;
    } else if (time_unit_ms > 1000) {
        xtp->xtimer_unit = 1000 * MSEC_TO_USEC;
    } else {
       xtp-> xtimer_unit = time_unit_ms * MSEC_TO_USEC;
    }

    /*
     * For better performance, we use bit shifting instead of division
     * in applying the time resolution.
     */
    xtp->xtimer_bits = find_max_bits(xtp->xtimer_unit);

    /*
     * Init the bucket chunk.
     */
    xtp->xtimer_chunk =
        chunk_header_init(sizeof(xtimer_bucket_t), 1,
                          CHUNK_FLAGS_MALLOC | CHUNK_FLAGS_ALIGN_64,
                          "xtimer bucket");
    if (xtp->xtimer_chunk == NULL) {
        return (-EINVAL);
    }

    switch(clock_type){
        case XTIMER_REALTIME:
            xtp->xtimer_clock_type = CLOCK_REALTIME;
            break;
        case XTIMER_MONOTONIC:
            xtp->xtimer_clock_type = CLOCK_MONOTONIC;
            break;
        case XTIMER_NOT_SPECIFIED:
            xtp->xtimer_clock_type = xtimer_get_tstamp_clock_type();
            break;
        default:
            return (-EINVAL);
   }
    /*
     * Variables for the timer thread.
     */
    if ((ret = pthread_mutex_init(&xtp->xtimer_w_mutex, NULL)) ||
        (ret = pthread_condattr_init(&xtimer_w_condattr)) ||
        (ret = pthread_condattr_setclock(&xtimer_w_condattr,
                                         xtp->xtimer_clock_type)) ||
        (ret = pthread_cond_init(&xtp->xtimer_w_cond, &xtimer_w_condattr)) ||
        (ret = pthread_condattr_destroy(&xtimer_w_condattr))) {
        (void) pthread_mutex_destroy(&xtp->xtimer_w_mutex);
        (void) pthread_cond_destroy(&xtp->xtimer_w_cond);
        (void) pthread_condattr_destroy(&xtimer_w_condattr);
        return (-ret);
    }

    /*
     * Init the tree for the xtimers.
     */
    rbtree_sh_init(&xtp->xtimer_tree,
                   &xtp->xtimer_tree.rbtree_common,
                   offsetof(xtimer_bucket_t, tm_expire),
                   sizeof(u_int64_t),
                   rbtree_uint64_cmp_func,
                   xtimer_node_free_func,
                   xtp);

    return (0);
}

/*
 * xtimer_glob_init
 *
 * Init the global variables for xtimer. The time_unit is in ms.
 *
 * Note that the time unit should be between (10, 1000) ms.
 */
int
xtimer_glob_init (u_int32_t time_unit_ms)
{
    int ret = 0;
    const xtimer_init_info_t info = {
        .time_unit_ms = time_unit_ms,
        .clock_type = XTIMER_NOT_SPECIFIED,
        .flags = default_xtimer_flags,
    };

    if ((xtimer_glob_init_v2(&info)) < 0) {
        ret = -1;
    }
    return (ret);
}

/*
 * xtimer_glob_init_v2
 *
 * Init the global variables for xtimer.
 *
 */
int
xtimer_glob_init_v2 (const xtimer_init_info_t *info_p)
{
    if (!info_p) {
        return (-EINVAL);
    }
    return (xtimer_pool_init_internal(&xtimer_default_pool,
                                      info_p->time_unit_ms,
                                      info_p->clock_type));
}

/*
 * xtimer_pool_create
 *
 * Create and initialize the variables for a new xtimer pool. The
 * time_unit is in ms.
 *
 * Note that the time unit should be between (10, 1000) ms.
 */
void* xtimer_pool_create (u_int32_t time_unit_ms)
{
    void *pool_p = NULL;

    const xtimer_init_info_t info = {
        .time_unit_ms = time_unit_ms,
        .clock_type = XTIMER_NOT_SPECIFIED,
        .flags = default_xtimer_flags,
    };

    if (xtimer_pool_create_v2(&info, &pool_p) < 0) {
        pool_p = NULL;
    }

    return (pool_p);
}

/*
 * xtimer_pool_create_v2
 *
 * Create and initialize the variables for a new xtimer pool.
 *
 */
int xtimer_pool_create_v2 (const xtimer_init_info_t *info_p,
                           void  **pool_p)
{
    int ret = 0;

    if (!info_p || !pool_p) {
        return (-EINVAL);
    }

    *pool_p = NULL;

    xtimer_pool_t *xtp = calloc(1, sizeof(*xtp));
    if (!xtp) {
        return (-ENOMEM);
    }
    ret = xtimer_pool_init_internal(xtp,
                                    info_p->time_unit_ms,
                                    info_p->clock_type);
    if (ret < 0) {
        free(xtp);
    } else {
       *pool_p = xtp;
    }

    return (ret);
}

void xtimer_pool_destroy (void** pool)
{
    xtimer_pool_t* xtp = (pool != NULL) ? *((xtimer_pool_t**) pool) : NULL;
    if (xtp != NULL) {
        free(xtp);
        *pool = NULL;
    }
}

/*
 * xtimer_pool_init
 *
 * Init a timer before using.
 */
void
xtimer_pool_init (
    void* pool UNUSED,
    xtimer_t *timer,
    void *object,
    u_int16_t obj_type,
    u_int8_t sub_type)
{
    memset(timer, 0, sizeof(xtimer_t));

    timer->object   = object;
    timer->obj_type = obj_type;
    timer->sub_type = sub_type;
}

/*
 * xtimer_stop_internal
 *
 * Stop a timer, assumes the lock is already taken.
 */
static void
xtimer_stop_internal (xtimer_pool_t* xtp, xtimer_t *tm)
{
    xtimer_bucket_t *bucket;
    int             result;

    if (tm->flags & XTIMER_FLAG_EXPIRED) {
        dbl_dequeue(&xtp->xtimer_expiredQ, tm);
        tm->flags &= ~XTIMER_FLAG_EXPIRED;
    } else if (tm->flags & XTIMER_FLAG_RUNNING) {
        result = rbtree_search(&xtp->xtimer_tree, &tm->tm_expire, (rbnode_t **) &bucket);
        assert(result == 0);

        dbl_dequeue(&bucket->timerQ, tm);
        if (dbl_queue_is_empty(&bucket->timerQ)) {
            rbtree_delete(&xtp->xtimer_tree, (rbnode_t *) bucket);
        }

        tm->flags &= ~XTIMER_FLAG_RUNNING;
        xtp->xtimer_curr_running--;
    }
}

/*
 * xtimer_stop
 *
 * Stop a timer.
 */
void
xtimer_pool_stop (void* pool, xtimer_t *tm)
{
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        xtimer_stop_internal(xtp, tm);
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
}

/*
 * xtimer_enqueue
 *
 * Enqueue a timer to bucket.
 */
static void
xtimer_enqueue (xtimer_pool_t* xtp, xtimer_bucket_t *bucket, xtimer_t *tm)
{
    dbl_enqueue(&bucket->timerQ, tm);
    tm->flags |= XTIMER_FLAG_RUNNING;
    if (++xtp->xtimer_curr_running > xtp->xtimer_max_running) {
        xtp->xtimer_max_running = xtp->xtimer_curr_running;
    }
}

/*
 * xtimer_start_internal
 *
 * Start a timer, assumes that the mutex lock has been already taken.
 */
static void
xtimer_start_internal (xtimer_pool_t* xtp, xtimer_t *tm, u_int64_t ms)
{
    xtimer_bucket_t *curr, *new;
    int             result;

    if (tm->flags & (XTIMER_FLAG_EXPIRED + XTIMER_FLAG_RUNNING)) {
        xtimer_stop_internal(xtp, tm);
    }

    /*
     * Be sure to apply the specified timer unit.
     */
    tm->tm_expire = ((xtimer_tstamp_us(xtp) + ms * ((u_int64_t)MSEC_TO_USEC)) >>
                     xtp->xtimer_bits) << xtp->xtimer_bits;
    result = rbtree_search(&xtp->xtimer_tree, &tm->tm_expire, (rbnode_t **) &curr);
    if (result == 0) {
        xtimer_enqueue(xtp, curr, tm);
        return;
    }

    new = chunk_alloc(xtp->xtimer_chunk, TRUE);
    if (!new) {
        return;
    }

    new->tm_expire = tm->tm_expire;
    rbtree_insert(&xtp->xtimer_tree, (rbnode_t *) new, (rbnode_t *) curr, result);
    xtimer_enqueue(xtp, new, tm);

    /*
     * If we are running a dedicated expiration thread, we might need
     * to wake it up if this bucket expires sooner than the timer it
     * was previously waiting for.
     */
    if (new->tm_expire < xtp->xtimer_w_expire) {
        /*
         * Un-lock the mutex if were running in NPTL mode. This is not
         * most optimized, but we should be hitting this condition very
         * rarely, as the timer should not expire while starting the
         * timer.
         */
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);

        xtimer_mutex_lock(&xtp->xtimer_w_mutex);
        pthread_cond_signal(&xtp->xtimer_w_cond);
        xtimer_mutex_unlock(&xtp->xtimer_w_mutex);

        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
    }
}

/*
 * xtimer_pool_start
 *
 * Start a timer.
 */
void
xtimer_pool_start (
    void* pool,
    xtimer_t *tm,
    u_int32_t ms)
{
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        xtimer_start_internal(xtp, tm, (u_int64_t) ms);
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
}

/*
 * xtimer_pool_start64
 *
 * This function is cloned from xtimer_start() to receive 64bit msec timer value
 * Start a timer
 */
void
xtimer_pool_start64 (
    void* pool,
    xtimer_t *tm,
    u_int64_t ms)
{
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        xtimer_start_internal(xtp, tm, ms);
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
}


/*
 * Returns timer jitter between 0 to the given percentage of the
 * expiration time.
 */
static u_int32_t
xtimer_jitter (u_int32_t expires, u_int16_t pct)
{
    if (pct >= 100) {
        return (expires);
    } else if (pct == 0) {
        /* if percent is 0, return a value of 0 to avoid a
         * divide (modulo) by zero error below.
         */
        return (0);
    }

    /* Check expires, return if it's zero to avoid more divide
    by zero errors - for EV164311*/

    if (expires == 0) {
        return (0);
    }

    /*
     * Avoid overflow.
     */
    if (expires > (UINT_MAX / 100)) {
        expires = (expires / 100) * pct;
    } else {
        expires = (expires * pct) / 100;
    }

    return (random() % expires);
}

/*
 * Start a timer with jitter.
 */
void
xtimer_pool_start_jitter (
    void* pool,
    xtimer_t *timer,
    u_int32_t ms,
    u_int16_t pct)
{
    ms -= xtimer_jitter(ms, pct);
    xtimer_pool_start(pool, timer, ms);
}

/*
 * xtimer_pool_expired
 *
 * Returns 1 if a timer is expired, otherwise 0.
 */
int
xtimer_pool_expired (void* pool, xtimer_t *tm)
{
    u_int64_t  now;
    xtimer_pool_t *xtp = (typeof(xtp)) pool;

    if (!xtp) {
        assert(((void)"xtimer_pool_expired called with NULL pool pointer", 0));
    }
    now = xtimer_tstamp_us(xtp);

    return (tm->tm_expire <= now);
}

/*
 * xtimer_pool_running
 *
 * Returns 1 if a timer is running, otherwise 0.
 */
int
xtimer_pool_running (void* pool, xtimer_t *tm)
{
    if (xtimer_pool_expired(pool, tm)) {
        return (0);
    }

    return ((tm->flags & XTIMER_FLAG_RUNNING) ? 1 : 0);
}

/*
 * xtimer_pool_active
 *
 * Returns 1 if the timer has been scheduled but not yet processed,
 * otherwise 0.
 */
int
xtimer_pool_active (void* pool UNUSED, xtimer_t *tm)
{
    return (tm->flags & (XTIMER_FLAG_EXPIRED | XTIMER_FLAG_RUNNING));
}

/*
 * xtimer_remaining
 *
 * Returns number of msec left before expiration. As the 32 bit
 * only covers 49 days, we assume there is no timer larger than
 * that!
 */
u_int32_t
xtimer_pool_remaining (void* pool, xtimer_t *timer)
{
    u_int64_t now, diff;
    xtimer_pool_t *xtp = (typeof(xtp)) pool;

    if (!xtp) {
        assert(((void)"xtimer_pool_remaining called with NULL pool pointer", 0));
    }
    now = xtimer_tstamp_us(xtp);
    diff = timer->tm_expire - now;
    if ((int64_t) diff < 0) {
        return (0);
    }

    return (diff / MSEC_TO_USEC);
}

/*
 * xtimer_pool_expired_count
 *
 * Return the number of expired timers in the global expiredQ
 */
u_int32_t
xtimer_pool_expired_count (void* pool)
{
    u_int32_t size = 0;
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        size = dbl_queue_size(&xtp->xtimer_expiredQ);
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
    return (size);
}

/*
 * xtimer_pool_running_count
 *
 * Return the number of running timers in the tree.
 */
u_int32_t
xtimer_pool_running_count (void* pool)
{
    u_int32_t xtimer_curr_running_local = 0;
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        xtimer_curr_running_local = xtp->xtimer_curr_running;
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
    return (xtimer_curr_running_local);
}

/*
 * xtimer_get_stats_internal
 *
 * Return the xtimer stats, assumes the lock is already taken.
 */
static void
xtimer_get_stats_internal (xtimer_pool_t* xtp, xtimer_stat_t *buf)
{
    if (xtp != NULL) {
        buf->curr_expired = dbl_queue_size(&xtp->xtimer_expiredQ);
        buf->max_expired  = xtp->xtimer_max_expired;
        buf->curr_running = xtp->xtimer_curr_running;
        buf->max_running  = xtp->xtimer_max_running;
    }
}

/*
 * xtimer_get_stats
 *
 * Return the xtimer stats.
 */
void
xtimer_pool_get_stats(void* pool, xtimer_stat_t *buf)
{
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        xtimer_get_stats_internal(xtp, buf);
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
}

/*
 * xtimer_expire_one_bucket
 *
 * Expire timers in one bucket.
 */
static void
xtimer_expire_one_bucket (xtimer_pool_t* xtp, xtimer_bucket_t *bucket)
{
    dbl_qhead_t   *globalQ, *one_expQ;
    xtimer_t      *timer;

    if (xtp != NULL) {
        globalQ  = &xtp->xtimer_expiredQ;
        one_expQ = &bucket->timerQ;

        /*
         * Link to global expired queue if there is any entry expired.
         */
        if (!dbl_queue_is_empty(one_expQ)) {
            if (dbl_queue_is_empty(globalQ)) {
                globalQ->head = one_expQ->head;
            } else {
                globalQ->tail->next = one_expQ->head;
                one_expQ->head->prev = globalQ->tail;
            }
            globalQ->tail = one_expQ->tail;
            globalQ->count += one_expQ->count;

            xtp->xtimer_curr_running -= one_expQ->count;
            if (dbl_queue_size(&xtp->xtimer_expiredQ) > xtp->xtimer_max_expired) {
                xtp->xtimer_max_expired = dbl_queue_size(&xtp->xtimer_expiredQ);
            }

            /*
             * Mask all the entries with global expired flag.
             */
            for (timer = (xtimer_t *) one_expQ->head; timer; timer = timer->next) {
                timer->flags &= ~XTIMER_FLAG_RUNNING;
                timer->flags |= XTIMER_FLAG_EXPIRED;
            }
        }

        dbl_queue_init(&bucket->timerQ);
        rbtree_delete(&xtp->xtimer_tree, (rbnode_t *) bucket);
    }
}

/*
 * xtimer_pool_next_expired
 *
 * Returns the next expired timer from the global expiredQ.
 *
 * NOTE: If you are running in NPTL mode, you might need to protect
 * against the case where another thread could free this timer.
 */
xtimer_t *
xtimer_pool_next_expired (void* pool)
{
    xtimer_t   *timer = NULL;
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        timer = dbl_dequeue_front(&xtp->xtimer_expiredQ);
        if (timer) {
            timer->flags &= ~XTIMER_FLAG_EXPIRED;
        }
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
    return (timer);
}

/*
 * xtimer_pool_next_expired_nptl_safe
 *
 * Returns the next expired timer from the global expiredQ.
 *
 * The callback function will be called before release the xtimer lock,
 * this is done to ensure that this timer is not stopped by any other
 * thread, while CB function is called. The CB should not do any xtimer
 * related calls, as it will lead to deadlocks.
 */
bool
xtimer_pool_next_expired_nptl_safe (
    void* pool,
    xtimer_expired_cb_t cb,
    void* key)
{
    xtimer_t   *timer;
    bool       rc = FALSE;
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        timer = dbl_dequeue_front(&xtp->xtimer_expiredQ);
        if (timer) {
            timer->flags &= ~XTIMER_FLAG_EXPIRED;
            cb(timer, key);
            rc = TRUE;
        }
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
    return (rc);
}

/*
 * xtimer_master_expire
 *
 * Expire the timers that have expired since the last time expiration
 * was performed.
 */
static void
xtimer_master_expire (xtimer_pool_t* xtp)
{
    xtimer_bucket_t  *bucket;
    u_int64_t        now;

    if (xtp != NULL) {
        now = xtimer_tstamp_us(xtp);

        while ((bucket =
                (xtimer_bucket_t *) rbtree_iterate_first(&xtp->xtimer_tree)) != NULL) {
            if (bucket->tm_expire <= now) {
                xtimer_expire_one_bucket(xtp, bucket);
            } else {
                break;
            }
        }
    }
}

/*
 * xtimer_master_expire_bucket
 *
 * Find the bucket that will expire next.
 */
static inline xtimer_bucket_t *
xtimer_master_expire_bucket (xtimer_pool_t* xtp)
{
    return ((xtimer_bucket_t*)
        ((xtp != NULL) ?
            rbtree_iterate_first(&xtp->xtimer_tree) :
            NULL));
}

/*
 * xtimer_pool_expired_wait
 *
 * Perform conditional timed wait and expire timers.
 */
int
xtimer_pool_expired_wait(void* pool)
{
    int              error, res;
    xtimer_bucket_t  *bucket;
    struct timespec  expts;
    struct timeval   now UNUSED;
    u_int64_t        gtd_expire UNUSED;
    xtimer_pool_t*   xtp = (xtimer_pool_t*) pool;

    if (xtp == NULL) {
        error = 0;
    }
    else {
        xtimer_mutex_lock(&xtp->xtimer_w_mutex);

        error = 0;

        /*
         * The default wait is 30 secs when there is no timer running.
         */
        while (TRUE) {

            bucket = xtimer_master_expire_bucket(xtp);
            if (bucket) {
                xtp->xtimer_w_expire = bucket->tm_expire;
            } else {
                xtp->xtimer_w_expire = xtimer_tstamp_us(xtp) + 30 * SEC_TO_USEC;
            }

    #ifndef USEC_TO_NSEC
    #define USEC_TO_NSEC    1000
    #endif

            expts.tv_sec = xtp->xtimer_w_expire / SEC_TO_USEC;
            expts.tv_nsec = (xtp->xtimer_w_expire % SEC_TO_USEC) * USEC_TO_NSEC;
            res = pthread_cond_timedwait(&xtp->xtimer_w_cond, &xtp->xtimer_w_mutex, &expts);

            xtp->xtimer_w_expire = 0;

            switch (res) {
              case 0:
                /*
                 * Another thread decreased the expiration time. We need to
                 * recalculate our sleep time.
                 */
                break;

              case ETIMEDOUT:
                /*
                 * master timer expired.
                 */
                goto exit;

              default:
                /*
                 * An unknown error occurred.
                 */
                error = res;
                goto exit;
            }
        }

    exit:

        /*
         * Expire the timers.
         */
        xtimer_master_expire(xtp);

        xtimer_mutex_unlock(&xtp->xtimer_w_mutex);
    }
    return (error);
}

/*
 * xtimer_pool_start_qlimited
 *
 * Start a timer with constrainted timer queue depth. If the queue depth
 * exceeds "queue_max", find the next timer bucket (that is no more than
 * timer_unit apart) that does not exceed such queue_max. When all the
 * buckets within the max_delay have full queues, the timer does not
 * get started.
 *
 * This function is used by PIM to smooth out the timer queues.
 */
void
xtimer_pool_start_qlimited (
    void* pool,
    xtimer_t* timer,
    u_int32_t ms,
    u_int32_t max_delay,
    u_int32_t max_qsize)
{
    xtimer_bucket_t  *prev, *curr;
    u_int64_t        now, tm_expire;
    int              result;
    xtimer_pool_t*   xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        /*
        * The tree may be changed as a result of stopping a timer.
        * So stop the timer before calling splay_find() here.
        */
        xtimer_stop_internal(xtp, timer);

        now = xtimer_tstamp_us(xtp);
        tm_expire = ((now + ms * ((u_int64_t)MSEC_TO_USEC)) >> xtp->xtimer_bits) << xtp->xtimer_bits;
        result = rbtree_search(&xtp->xtimer_tree, &tm_expire, (rbnode_t **) &curr);
        if (result != 0) {
            xtimer_start_internal(xtp, timer, (u_int64_t) ms);

            xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
            return;
        }

        prev = NULL;
        while (curr &&
            ((curr->tm_expire - tm_expire) < (max_delay * ((u_int64_t)MSEC_TO_USEC)))) {
            if (!prev || ((curr->tm_expire - prev->tm_expire) <= xtp->xtimer_unit)) {
                if (dbl_queue_size(&curr->timerQ) < (int) max_qsize) {
                    timer->tm_expire = curr->tm_expire;
                    xtimer_enqueue(xtp, curr, timer);
                    break;
                }
            } else {
                xtimer_start_internal(
                    xtp,
                    timer,
                    (u_int64_t)
                        ((prev->tm_expire - now + xtp->xtimer_unit)
                         / MSEC_TO_USEC));
                break;
            }

            prev = curr;
            curr = (xtimer_bucket_t *) rbtree_iterate_next_sh_mem_safe(
                                                                &xtp->xtimer_tree,
                                                                (rbnode_t *)curr);
        }
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
}

/*
 * xtimer_get_expired_timers
 *
 * For backend process to get expired xtimers
 */
static int
xtimer_get_expired_timers (xtimer_pool_t* xtp, xtimer_show_request_t *req)
{
    xtimer_t               *xtimer_pt;
    xtimer_show_response_t *resp_pt;
    int                    total_len =
        (xtp == NULL) ?
            0 :
            sizeof(xtimer_show_response_t);
    xtimer_show_elem_t     *show_xtimer;
    struct timeval         now;

    if (xtp != NULL) {
        resp_pt = (xtimer_show_response_t *)req;
        resp_pt->no_more = TRUE;
        resp_pt->count   = 0;
        show_xtimer      = resp_pt->xtimer;
        rbn_gettstamp_tv(&now);
        resp_pt->curr_time.tv_sec = now.tv_sec;
        resp_pt->curr_time.tv_usec = now.tv_usec;

        xtimer_pt = (xtimer_t *) xtp->xtimer_expiredQ.head;

        /* only get the timers that fit into one cli buf */
        for (xtimer_pt = (xtimer_t *) xtp->xtimer_expiredQ.head;
            xtimer_pt && (total_len + sizeof(xtimer_show_elem_t) <=
                        MO_MAX_OBJSIZE);
            xtimer_pt = (xtimer_t *) xtimer_pt->next) {
            total_len += sizeof(xtimer_show_elem_t);
            show_xtimer->tm_expire = xtimer_pt->tm_expire;
            show_xtimer->obj_type  = xtimer_pt->obj_type;
            show_xtimer->sub_type  = xtimer_pt->sub_type;
            show_xtimer->flags     = xtimer_pt->flags;
            resp_pt->count++;
            show_xtimer++;
        }
    }
    return (total_len);
}

/*
 * xtimer_get_running_timers
 *
 * For backend process to get running xtimers
 */
static int
xtimer_get_running_timers (xtimer_pool_t* xtp, xtimer_show_request_t *req)
{
    xtimer_show_response_t *resp_pt;
    xtimer_bucket_t        *bucket;
    xtimer_t               *xtimer_pt;
    xtimer_show_elem_t     *show_xtimer;
    int                    total_len =
        (xtp == NULL) ?
            0 :
            sizeof(xtimer_show_response_t);
    int                    tmp_len;
    struct timeval         now;

    if (xtp != NULL) {
        resp_pt = (xtimer_show_response_t *)req;
        resp_pt->no_more = TRUE;
        resp_pt->count   = 0;
        show_xtimer      = resp_pt->xtimer;
        rbn_gettstamp_tv(&now);
        resp_pt->curr_time.tv_sec = now.tv_sec;
        resp_pt->curr_time.tv_usec = now.tv_usec;
        if (req->start_key) {
            bucket = (xtimer_bucket_t *) rbtree_lookup_ge(&xtp->xtimer_tree,
                                                        &(req->start_key));
        } else {
            bucket = (xtimer_bucket_t *) rbtree_iterate_first(&xtp->xtimer_tree);
        }
        while (bucket) {
            /* check wether timers in this bucket fit in the current cli buf */
            tmp_len = dbl_queue_size(&(bucket->timerQ)) *
                sizeof(xtimer_show_elem_t);
            if (total_len + tmp_len > MO_MAX_OBJSIZE) {
                if (total_len > (int) sizeof(xtimer_show_response_t)) {
                    resp_pt->next_key = bucket->tm_expire;
                    resp_pt->no_more   = FALSE;
                    return (total_len);
                }
            }

            for (xtimer_pt = (xtimer_t *) bucket->timerQ.head;
                xtimer_pt && (total_len + sizeof(xtimer_show_elem_t) <=
                            MO_MAX_OBJSIZE);
                xtimer_pt = (xtimer_t *) xtimer_pt->next) {
                total_len += sizeof(xtimer_show_elem_t);
                show_xtimer->tm_expire = xtimer_pt->tm_expire;
                show_xtimer->obj_type  = xtimer_pt->obj_type;
                show_xtimer->sub_type  = xtimer_pt->sub_type;
                show_xtimer->flags     = xtimer_pt->flags;
                resp_pt->count++;
                show_xtimer++;
            }
            bucket = (xtimer_bucket_t *) rbtree_iterate_next_sh_mem_safe(
                                                            &xtp->xtimer_tree,
                                                            (rbnode_t *)bucket);
        }
    }
    return (total_len);
}

/*
 * xtimer_get_info
 *
 * For backend process to get xtimer info:
 * global stats, expired timers and running timers.
 */
int
xtimer_pool_get_info (void* pool, moObjectHead *object)
{
    int                   len = 0;
    xtimer_show_request_t *request;
    xtimer_pool_t* xtp = (xtimer_pool_t*) pool;

    if (xtp != NULL) {
        xtimer_nptlonly_mutex_lock(&xtp->xtimer_w_mutex);
        request = (xtimer_show_request_t *)object;
        switch (request->request_type) {
          case XTIMER_SHOW_GLOBAL_STATS:
            xtimer_get_stats_internal(xtp, (xtimer_stat_t *)request);
            len = sizeof(xtimer_stat_t);
            break;

          case XTIMER_SHOW_EXPIRED:
            len = xtimer_get_expired_timers(xtp, request);
            break;

          case XTIMER_SHOW_RUNNING:
            len = xtimer_get_running_timers(xtp, request);
            break;

          default:
            break;
        }
        xtimer_nptlonly_mutex_unlock(&xtp->xtimer_w_mutex);
    }
    return (len);
}

/*
 * xtimer_pool_print_stats
 *
 * for CLI to print xtimer global stats
 */
void
xtimer_pool_print_stats(void* pool UNUSED, xtimer_stat_t * stat_pt)
{
    if (stat_pt) {
        printf("%20s %-20s\n", "-- XTIMER", "STATS --");
        printf("%-16s: %-8i    %-15s: %-8i\n",
               "# of expired", stat_pt->curr_expired,
               "max expired", stat_pt->max_expired);
        printf("%-16s: %-8i    %-15s: %-8i\n\n",
               "# of running", stat_pt->curr_running,
               "max running", stat_pt->max_running);
    }
}

/*
 * xtimer_pool_print_stats
 *
 * for CLI to print individual xtimer info
 */
void
xtimer_pool_print_timer_info (void* pool UNUSED, xtimer_show_elem_t *xtimer_pt, u_int64_t curr_time)
{
    u_int32_t      secs, msecs;
    bool           expired = FALSE;

    if (xtimer_pt) {
        if (xtimer_pt->tm_expire >= curr_time) {
            xtimer_pt->tm_expire -= curr_time;
        } else {
            xtimer_pt->tm_expire = curr_time - xtimer_pt->tm_expire;
            expired = TRUE;
        }
        secs = xtimer_pt->tm_expire / SEC_TO_USEC;
        msecs = (xtimer_pt->tm_expire / SEC_TO_MSEC) % SEC_TO_MSEC;
        printf("type: %-3i subtype: %-2i flag: 0x%-2x",
               xtimer_pt->obj_type, xtimer_pt->sub_type, xtimer_pt->flags);
        if (expired == TRUE) {
            printf(" expired %i sec %i msec ago\n\n",
                   secs, msecs);
        } else {
            printf(" expires in %i sec %i msec\n\n",
                   secs, msecs);
        }
    }
}
