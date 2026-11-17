#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <CommonCrypto/CommonDigest.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#define MAX_DIGITS 16
//#define CC_SHA256_DIGEST_LENGTH 32 

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

        if (atomic_load(&sh->found))//сheck if found 
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

static void generate_pin(char *pin, int d){
    for(int i = 0; i < d; i++){
        pin[i] = (rand()%10 + '0');
    }
    pin[d] = '\0';
}

static bool parse_int(const char *s, long min, long max, long *out)
{
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);

    if(end == s || *end != '\0' || errno == ERANGE || v > max || v < min){ 
        return false;
    }

    *out = v; //output 
    return true;
}

static void print_hash(const unsigned char *hash){
    printf("HASH: ");
    for(int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++){
        printf("%02x", hash[i]);
    }
    printf("\n");
}

int main(int argc, char **argv){

    srand(time(NULL));

    if(argc != 3){
        printf("Wrong number of arguments\n");
        return 1;
    }

    long d, t;
    if (!parse_int(argv[1], 4, MAX_DIGITS, &d)) {
        printf("Error: d must be an integer from 4 to 16\n");
        return 1;
    }
    if (!parse_int(argv[2], 1, 512, &t)) {
        printf("Error: t must be an integer from 1 to 512\n");
        return 1;
    }

    unsigned long long n = 1;
    for (int i = 0; i < d; i++){ //num of candidates
        n *= 10;
    }

    char pin[MAX_DIGITS + 1];
    generate_pin(pin, (int)d);
    //printf("PIN: %s\n", pin);

    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(pin, d, hash);
    print_hash(hash);

    struct shared sh;
    sh.target = hash;
    sh.d = (int)d;
    atomic_init(&sh.found, false);

    pthread_t *threads = malloc(t * sizeof *threads);
    struct task *tasks = malloc(t * sizeof *tasks);
    if (threads == NULL || tasks == NULL) {
        printf("Error: out of memory\n");
        free(threads);
        free(tasks);
        return 1;
    }

    int created = 0;
    for (int k = 0; k < t; k++) {
        tasks[k].sh = &sh;
        tasks[k].lo = (unsigned long long)k * n / t;
        tasks[k].hi = (unsigned long long)(k + 1) * n / t;
        if (pthread_create(&threads[k], NULL, worker, &tasks[k]) != 0) { //if the output is non-zero -> error
            printf("Error: could not create thread %d\n", k);
            atomic_store(&sh.found, true); //to stop all the working ones
            break;
        }
        created++;
    }

    for (int k = 0; k < created; k++){
        pthread_join(threads[k], NULL); //the result is already in sh.result so NULL
    }

    free(threads);
    free(tasks);

    if (created < t)
        return 1;
    if (!atomic_load(&sh.found)) {
        printf("PIN not found\n");
        return 1;
    }
    printf("Cracked PIN: %s\n", sh.result);
    return 0;
}

