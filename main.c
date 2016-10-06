#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>


#include IMPL
#include "thread.h"
#include "field_alignment.c"


#define DICT_FILE "./dictionary/words.txt"
#define ALIGN_FILE "aligned.txt"



static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

int main(int argc, char *argv[])
{
    struct timespec start, end;
    double cpu_time1, cpu_time2;

#ifndef OPT
    int i = 0;
    char line[MAX_LAST_NAME_SIZE];
    FILE *fp;
    /* check file opening */
    fp = fopen(DICT_FILE, "r");
    if (!fp) {
        printf("cannot open the file\n");
        return -1;
    }
#endif

    /* build the entry */
    entry *pHead, *e;
    pHead = (entry *) malloc(sizeof(entry));
    printf("size of entry : %lu bytes\n", sizeof(entry));
    e = pHead;
    e->pNext = NULL;

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif

#if defined(OPT)

    file_align(DICT_FILE, ALIGN_FILE, MAX_LAST_NAME_SIZE);
    int fd = open(ALIGN_FILE, O_RDONLY | O_NONBLOCK);
    off_t fs = fsize(ALIGN_FILE);

    clock_gettime(CLOCK_REALTIME, &start);

    char *map = mmap(NULL, fs, PROT_READ, MAP_SHARED, fd, 0);

    assert(map && "mmap error");

    /* allocate at beginning */
    entry *entry_pool = (entry *) malloc(sizeof(entry) * fs / MAX_LAST_NAME_SIZE);
    assert(entry_pool && "entry_pool error");

    pthread_setconcurrency(THREAD_NUM + 1);

    /* Malloc for Pthread id and args */
    pthread_t *tid = (pthread_t *) malloc(sizeof(pthread_t) * THREAD_NUM);
    
        pthread_mutex_t lock;
        pthread_mutex_init(&lock, NULL);
    
        threadpool_t *pool = threadpool_create(THREAD_NUM, 512 , 0); 
        append_a **app = (append_a **) malloc(sizeof(append_a *) * THREAD_NUM);

        //pthread_mutex_lock(&lock);
        for( int i = 0 ; i < THREAD_NUM ; i++ ){
        app[i] = new_append_a(map + MAX_LAST_NAME_SIZE * i,map + fs, i, THREAD_NUM, entry_pool + i);
        //pthread_mutex_unlock(&lock);
    }

        for( int i = 0 ; i < THREAD_NUM ; i++ ){
            threadpool_add(pool, &append, (void *)app[i], 0);
        }

    /* Graceful shutdown */
    threadpool_destroy(pool, 1);
    //pthread_mutex_destory(&lock);

    entry *etmp = pHead;
    pHead = app[0]->pHead;
    etmp = app[0]->pLast;

    for (int i = 1; i < THREAD_NUM; i++) {
        etmp->pNext = app[i]->pHead;
        etmp = app[i]->pLast;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

#else
    clock_gettime(CLOCK_REALTIME, &start);
    while (fgets(line, sizeof(line), fp)) {
        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;
        e = append(line, e);
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

    /* close file as soon as possible */
    fclose(fp);
#endif

#if defined COMPARE
    e = pHead;
    FILE *fp = fopen("entry_words.txt","w+");
    while (e) {
        fprintf(fp, "%s", e->lastName);
        e = e->pNext;
    }
    fclose(fp);
    compare("entry_words.txt",DICT_FILE);
#endif

    e = pHead;

    /* the givn last name to find */
    char input[MAX_LAST_NAME_SIZE] = "zyxel";

    assert(findName(input, e) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e)->lastName, "zyxel"));

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    /* compute the execution time */
    clock_gettime(CLOCK_REALTIME, &start);
    findName(input, e);
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time2 = diff_in_second(start, end);

    FILE *output;
#if defined(OPT)
    output = fopen("opt.txt", "a");
#else
    output = fopen("orig.txt", "a");
#endif
    fprintf(output, "append() findName() %lf %lf\n", cpu_time1, cpu_time2);
    fclose(output);

    printf("execution time of append() : %lf sec\n", cpu_time1);
    printf("execution time of findName() : %lf sec\n", cpu_time2);

#ifndef OPT
    while (pHead->pNext) free(pHead->pNext);
    free(pHead);
#else
    free(entry_pool);
    free(tid);
    free(app);
    munmap(map, fs);
#endif
    return 0;
}