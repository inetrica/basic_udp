#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

void usage() {
    fprintf(stderr, "Usage: blaster -s <hostname> -p <port> -r <rate> "
    "-n <num packets> -q <seq_no> -l <length> -c <echo>\n");
    exit(1);
}

void invalidRange(char* option){
    fprintf(stderr, "Error: Invalid range value for %s\n", option);
    exit(1);
}

unsigned int getUint(char* option, char* arg){
    unsigned long tmp = 0;
    errno = 0;
    tmp = strtoul(arg, NULL, 10);
    if(errno != 0 || tmp > UINT_MAX){
        invalidRange(option);
    }

    return (uint) tmp;

}

void getargs(char **hostname, int *port, int *rate, int *num, 
        uint *seq_no, uint *length, int *echo, int argc, char *argv[]){
    int a = 0;
    int c = 0;
    while((a = getopt(argc, argv, "s:p:r:n:q:l:c:")) != -1){
        c++;
        switch(a){
            case 's':
                *hostname = strdup(optarg);
                break;
            case 'p':
                *port = atoi(optarg);
                if(*port <= 1024 || 65536 <= *port) invalidRange("port");
                break;
            case 'r':
                *rate = atoi(optarg);
                if(*rate <= 0) invalidRange("rate");
                break;
            case 'n':
                *num = atoi(optarg);
                if(*num < 0) invalidRange("num");
                break;
            case 'q':
                *seq_no = getUint("seq_no", optarg);
                //if(*seq_no < 0) invalidRange("seq_no");
                break;
            case 'l':
                *length = getUint("length", optarg);
                //if(*length < 0 || 50000 <= *length) invalidRange("length");
                break;
            case 'c':
                *echo = atoi(optarg);
                if(*echo != 0 && *echo != 1) invalidRange("echo");
                break;
            default:
                usage();
        }
    }
    if(c != 7) usage();

}

/*
 * calculate amount of time we'll need to sleep in order to match 'rate'
 * where rate is pkts/sec
 * */
struct timespec calcSleepTime(int rate){
    const uint nsps = 1000000000; //1000000000 nanoseconds per second
    struct timespec tspec;
    long nanosecSleep = nsps/rate;
    int sec = nanosecSleep/nsps;
    tspec.tv_sec = sec;
    tspec.tv_nsec = nanosecSleep%nsps;
    return tspec;
}

int main(int argc, char *argv[]){

    char* hostName = NULL;
    int port, rate, numPkts, echo;
    uint seq_no, len;
    getargs(&hostName, &port, &rate, &numPkts, &seq_no, &len, &echo, argc, argv);   

    fprintf(stdout, "hostName = %s\nport = %d\nrate = %d\nnum = %d\nseq = %u\nlen = %u\necho = %d\n",
            hostName, port, rate, numPkts, seq_no, len, echo);

    struct timespec ts = calcSleepTime(rate);
    fprintf(stdout, "sleep rate is sec = %d, nano = %ld\n", (int) ts.tv_sec, ts.tv_nsec);

    int i;
    for(i = 0; i < numPkts; i++){
        fprintf(stdout, "send a packet\n");
        nanosleep(&ts, NULL);
    }

    exit(0);

}
