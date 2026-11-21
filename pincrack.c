#include "search.h"
#include <CommonCrypto/CommonDigest.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define REPS 5

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

static double now_sec()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

static int run_bench(int d, int tmax)
{
    char pin[MAX_DIGITS + 1];
    for (int i = 0; i < d; i++)
        pin[i] = '9';
    pin[d] = '\0';

    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(pin, d, hash);
    printf("t,median_s,speedup\n");

    double t1 = 0;

    for (int t = 1; t <= tmax; t++) {
        double times[REPS];
        char result[MAX_DIGITS + 1];

        for (int r = 0; r < REPS; r++) {
            double start = now_sec();
            int rc = crack_parallel(hash, d, t, result);
            times[r] = now_sec() - start;

            if (rc != 0 || strcmp(result, pin) != 0) {
                printf("Error: run failed for t=%d\n", t);
                return 1;
            }
        }

        qsort(times, REPS, sizeof(double), cmp_double);
        double median = times[REPS / 2];
        if (t == 1){
            t1 = median;
        }
        printf("%d,%.6f,%.3f\n", t, median, t1 / median);
        fflush(stdout);
    }
    return 0;
}







int main(int argc, char **argv){

    srand(time(NULL));

    if (argc == 4 && strcmp(argv[1], "--bench") == 0) {
        long d, tmax;
        if (!parse_int(argv[2], 4, MAX_DIGITS, &d) || !parse_int(argv[3], 1, 512, &tmax)) {
            printf("Error: usage: %s --bench <d> <tmax>\n", argv[0]);
            return 1;
        }
        return run_bench(d, tmax);
    }

    if(argc != 3){
        printf("Error: wrong number of arguments\n");
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

    char pin[MAX_DIGITS + 1];
    generate_pin(pin, d);
    //printf("PIN: %s\n", pin);

    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(pin, d, hash);
    print_hash(hash);

    char result[MAX_DIGITS + 1];
    double start = now_sec();
    int rc = crack_parallel(hash, d, t, result);
    double elapsed = now_sec() - start;

    if (rc == -1)
        return 1; //error
    if (rc == 1) {
        printf("PIN not found\n");
        return 1;
    }
    printf("Cracked PIN: %s\n", result);
    printf("Time: %.6f s\n", elapsed);
    return 0;
}