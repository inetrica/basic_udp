#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define NANO 1000000000

/*
 * if user runs program with an arg which has an out of range value
 * print msg
 */
void invalidRange(char* option){
    fprintf(stderr, "Error: Invalid range value for %s\n", option);
    exit(1);
}

/*
 * print error msg and exit
 */
void err(char* msg){
    fprintf(stderr, msg);
    exit(1);
}

/*
 * subtract time b from time a
 * ie return a - b in terms of time
 */
struct timespec
subtract(struct timespec a, struct timespec b){
    struct timespec tmp;
    //if time between nanosecs is < 0, need to
    //take time away from secs, add back to nsec
    if((b.tv_nsec - a.tv_nsec) < 0){
        //carry over
        tmp.tv_sec = (b.tv_sec - a.tv_sec) - 1;
        tmp.tv_nsec = (b.tv_nsec - a.tv_nsec) + NANO;
    } else {
        tmp.tv_sec = (b.tv_sec - a.tv_sec);
        tmp.tv_nsec = (b.tv_nsec - a.tv_nsec);
    }
    return tmp;
}
