/* lock up */
if (sync != 0) {
    /* take time */
    struct timespec before_lock;
    m_clock_gettime(CLOCK_REALTIME, &before_lock);

    if (sync == 'm') {
        m_pthread_mutex_lock(&mutex);
    }
    else if (sync == 's') {
        while (__sync_lock_test_and_set(&spin_lock, 1));
    }

    /* take time */
    struct timespec after_lock;        
    m_clock_gettime(CLOCK_REALTIME, &after_lock);
    /* calculate time elapsed */
    long long lock_time = after_lock.tv_nsec - before_lock.tv_nsec;
    lock_time += (after_lock.tv_sec - before_lock.tv_sec) * 1000000000;
    /* add this time to counter array */
    if (lock_time_arr && lock_ops_arr) { /* only if these 2 have been initialized */
        lock_time_arr[my_id] += lock_time;
        lock_ops_arr[my_id] += 1;
    }
}