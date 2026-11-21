#include "search.h"
#include <CommonCrypto/CommonDigest.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct shared {
    const unsigned char *target;
    int d;
    atomic_bool found; //atomic_bool is used to prevent the data race and update the flag live
    char result[MAX_DIGITS + 1];
};

struct task {
    struct shared *sh;
    unsigned long long lo, hi;
};

static void *worker(void *arg)
{
    struct task *task = arg;
    struct shared *sh = task->sh;
    int d = sh->d;

    char guess[MAX_DIGITS + 1];
    guess[d] = '\0';
    unsigned char h[CC_SHA256_DIGEST_LENGTH];

    for (unsigned long long i = task->lo; i < task->hi; i++) {

        if (atomic_load(&sh->found)) //check if found
            break;

        unsigned long long x = i;
        for (int j = 0; j < d; j++) {
            guess[d - j - 1] = (char)(x % 10 + '0');
            x /= 10;
        }

        CC_SHA256(guess, d, h);
        if (memcmp(h, sh->target, CC_SHA256_DIGEST_LENGTH) == 0) {
            memcpy(sh->result, guess, d + 1); //d + 1 bc of '\0'
            atomic_store(&sh->found, true);
            break;
        }
    }

    return NULL;
}

int crack_parallel(const unsigned char *target, int d, int t, char *result)
{
    unsigned long long n = 1;
    for (int i = 0; i < d; i++) { //num of candidates = 10^d
        n *= 10;
    }

    struct shared sh;
    sh.target = target;
    sh.d = d;
    atomic_init(&sh.found, false);

    pthread_t *threads = malloc(t * sizeof *threads);
    struct task *tasks = malloc(t * sizeof *tasks);
    if (threads == NULL || tasks == NULL) {
        fprintf(stderr, "Error: out of memory\n");
        free(threads);
        free(tasks);
        return -1;
    }

    int created = 0;
    for (int k = 0; k < t; k++) {
        tasks[k].sh = &sh;
        tasks[k].lo = (unsigned long long)k * n / t;
        tasks[k].hi = (unsigned long long)(k + 1) * n / t;
        if (pthread_create(&threads[k], NULL, worker, &tasks[k]) != 0) { //if the output is non-zero -> error
            fprintf(stderr, "Error: could not create thread %d\n", k);
            atomic_store(&sh.found, true); //to stop all the working ones
            break;
        }
        created++;
    }

    for (int k = 0; k < created; k++) {
        pthread_join(threads[k], NULL); //the result is already in sh.result so NULL
    }

    free(threads);
    free(tasks);

    if (created < t)
        return -1; //error: not all threads were created
    if (!atomic_load(&sh.found))
        return 1; 
    memcpy(result, sh.result, d + 1); //sh dies after return, so copy the PIN out
    return 0;
}
