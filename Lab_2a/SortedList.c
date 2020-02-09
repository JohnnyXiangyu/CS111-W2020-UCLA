#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "SortedList.h"

extern pthread_mutex_t mutex;
extern volatile int lock;
extern SortedList_t* head;

/* safe wrap of pthread_mutex_lock */
int m_pthread_mutex_lock(pthread_mutex_t *__mutex) {
    int rc = pthread_mutex_lock(__mutex);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_lock() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* safe wrap of pthread_mutex_unlock */
int m_pthread_mutex_unlock(pthread_mutex_t *__mutex) {
    int rc = pthread_mutex_unlock(__mutex);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_unlock() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}

/* safe wrap of pthread_mutex_init */
int m_pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr) {
    int rc = pthread_mutex_init(__mutex, __mutexattr);
    if (rc != 0) {
        fprintf(stderr, "ERROR: pthread_mutex_init() returned %d, exiting ...\n", rc);
        exit(1);
    }
    else 
        return rc;
}


/* insert a node with previous and next give */
int insertBetween(SortedListElement_t* pre, SortedListElement_t* nex, SortedListElement_t* new) {
    new->prev = pre;
    new->next = nex;

    pre->next = new;
    nex->prev = new;

    return 0;
}


/* delete a given node */
int deleteMe(SortedListElement_t* element) {
    element->prev->next = element->next;
    element->next->prev = element->prev;

    return 0;
}


void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    SortedListElement_t* temp_node = list->next;
    SortedListElement_t* pre = temp_node;
    SortedListElement_t* nex = temp_node;
    while (temp_node != list && strcmp(element->key, temp_node->key) >= 0) {
        pre = temp_node;
        temp_node = temp_node->next;
        nex = temp_node;
    }
    if (opt_yield & INSERT_YIELD) {
        sched_yield();
    }
    insertBetween(pre, nex, element);
}


int SortedList_delete( SortedListElement_t *element) {
    if (element && element->next->prev == element->prev->next
            && element->prev->next == element /* check consistency */
            && element != head) /* check it's not deleting head */ {
        if (opt_yield & DELETE_YIELD) {
            sched_yield();
        }
        return deleteMe(element);
    }
    else
        return 1;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    SortedList_t* temp = list;
    if (list) {
        temp = list->next;
        while (temp != list && strcmp(temp->key, key) != 0) {
            if (opt_yield & LOOKUP_YIELD) {
                sched_yield();
            }
            temp = temp->next;
        }
    }
    if (temp == list) {
        return NULL;
    }
    else {
        return temp;
    }
}


int SortedList_length(SortedList_t *list) {
    if (! list) return -1;

    SortedList_t* temp = list;
    SortedList_t* pre = list;
    long long count = 1;
    while (temp && temp->next != list) {
        if (opt_yield & LOOKUP_YIELD) {
            sched_yield();
        }
        if (temp->key == NULL) 
            count --;
        pre = temp;
        temp = temp-> next;
        count ++;
        /* check corruption */
        if (!temp || temp->prev != pre)
            return -1;
    }
    return count;
}
