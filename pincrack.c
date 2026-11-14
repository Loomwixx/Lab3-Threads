#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <CommonCrypto/CommonDigest.h>

#define MAX_DIGITS 16
#define CC_SHA256_DIGEST_LENGTH 32

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

    char pin[MAX_DIGITS + 1];
    generate_pin(pin, d);
    //printf("PIN: %s\n", pin);

    unsigned char hash[32];

    CC_SHA256(pin, d, hash);
    print_hash(hash);
    return 0;
}
