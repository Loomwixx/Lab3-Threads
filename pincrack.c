#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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

int main(int argc, char **argv){
    if(argc != 3){
        printf("Wrong number of arguments\n");
        return 1;
    }
    long d, t;
    if (!parse_int(argv[1], 4, 16, &d)) {
        printf("Error: d must be an integer from 4 to 16\n");
        return 1;
    }
    if (!parse_int(argv[2], 1, 512, &t)) {
        printf("Error: t must be an integer from 1 to 512\n");
        return 1;
    }

    return 0;
}
