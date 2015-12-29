#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>
#include <sys/time.h>
#include "multi_skiplist.h"
#include "logger.h"
#include "shared_func.h"

#define COUNT 1280000
#define LEVEL_COUNT 18
#define MIN_ALLOC_ONCE  32
#define LAST_INDEX (COUNT - 1)

static int *numbers;
static MultiSkiplist sl;
static MultiSkiplistIterator iterator;

static int compare_func(const void *p1, const void *p2)
{
    return *((int *)p1) - *((int *)p2);
}

static int test_insert()
{
    int i;
    int result;
    int64_t start_time;
    int64_t end_time;
    void *value;

    start_time = get_current_time_ms();
    for (i=0; i<COUNT; i++) {
        if ((result=multi_skiplist_insert(&sl, numbers + i)) != 0) {
            return result;
        }
    }
    end_time = get_current_time_ms();
    printf("insert time used: %"PRId64" ms\n", end_time - start_time);

    start_time = get_current_time_ms();
    for (i=0; i<COUNT; i++) {
        value = multi_skiplist_find(&sl, numbers + i);
        assert(value != NULL && *((int *)value) == numbers[i]);
    }
    end_time = get_current_time_ms();
    printf("find time used: %"PRId64" ms\n", end_time - start_time);

    start_time = get_current_time_ms();
    i = 0;
    multi_skiplist_iterator(&sl, &iterator);
    while ((value=multi_skiplist_next(&iterator)) != NULL) {
        i++;
        assert(i == *((int *)value));
    }
    assert(i==COUNT);

    end_time = get_current_time_ms();
    printf("iterator time used: %"PRId64" ms\n", end_time - start_time);
    return 0;
}

static void test_delete()
{
    int i;
    int64_t start_time;
    int64_t end_time;
    void *value;

    start_time = get_current_time_ms();
    for (i=0; i<COUNT; i++) {
        assert(multi_skiplist_delete(&sl, numbers + i) == 0);
    }
    end_time = get_current_time_ms();
    printf("delete time used: %"PRId64" ms\n", end_time - start_time);

    start_time = get_current_time_ms();
    for (i=0; i<COUNT; i++) {
        value = multi_skiplist_find(&sl, numbers + i);
        assert(value == NULL);
    }
    end_time = get_current_time_ms();
    printf("find after delete time used: %"PRId64" ms\n", end_time - start_time);

    i = 0;
    multi_skiplist_iterator(&sl, &iterator);
    while ((value=multi_skiplist_next(&iterator)) != NULL) {
        i++;
    }
    assert(i==0);
}

typedef struct record
{
    int line;
    int key;
} Record;

static int compare_record(const void *p1, const void *p2)
{
    return ((Record *)p1)->key - ((Record *)p2)->key;
}

static int test_stable_sort()
{
#define RECORDS 20 
    int i;
    int result;
    int index1;
    int index2;
    MultiSkiplist sl;
    MultiSkiplistIterator iterator;
    Record records[RECORDS];
    Record *record;
    void *value;

    result = multi_skiplist_init_ex(&sl, 12, compare_record, 128);
    if (result != 0) {
        return result;
    }

    for (i=0; i<RECORDS; i++) {
        records[i].line = i + 1;
        records[i].key = i + 1;
    }

    for (i=0; i<RECORDS/4; i++) {
        index1 = (RECORDS - 1) * (int64_t)rand() / (int64_t)RAND_MAX;
        index2 = RECORDS - 1 - index1;
        if (index1 != index2) {
            records[index1].key = records[index2].key;
        }
    }

    for (i=0; i<RECORDS; i++) {
        if ((result=multi_skiplist_insert(&sl, records + i)) != 0) {
            return result;
        }
    }

    for (i=0; i<RECORDS; i++) {
        value = multi_skiplist_find(&sl, records + i);
        assert(value != NULL && ((Record *)value)->key == records[i].key);
    }

    i = 0;
    multi_skiplist_iterator(&sl, &iterator);
    while ((value=multi_skiplist_next(&iterator)) != NULL) {
        i++;
        record = (Record *)value;
        printf("%d => #%d\n", record->key, record->line);
    }
    assert(i==RECORDS);

    multi_skiplist_destroy(&sl);
    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    int tmp;
    int index1;
    int index2;
    int result;

    log_init();
    numbers = (int *)malloc(sizeof(int) * COUNT);
    srand(time(NULL));
    for (i=0; i<COUNT; i++) {
        numbers[i] = i + 1;
    }

    for (i=0; i<COUNT; i++) {
        index1 = LAST_INDEX * (int64_t)rand() / (int64_t)RAND_MAX;
        index2 = LAST_INDEX * (int64_t)rand() / (int64_t)RAND_MAX;
        if (index1 == index2) {
            continue;
        }
        tmp = numbers[index1];
        numbers[index1] = numbers[index2];
        numbers[index2] = tmp;
    }

    fast_mblock_manager_init();
    result = multi_skiplist_init_ex(&sl, LEVEL_COUNT, compare_func, MIN_ALLOC_ONCE);
    if (result != 0) {
        return result;
    }

    test_insert();
    printf("\n");

    fast_mblock_manager_stat_print(false);

    test_delete();
    printf("\n");

    test_insert();
    printf("\n");

    test_delete();
    printf("\n");
    multi_skiplist_destroy(&sl);

    test_stable_sort();

    printf("pass OK\n");
    return 0;
}
