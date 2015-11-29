#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

#define MAX_LEN 50000

void usage() {
    fprintf(stderr, "Usage: blaster -s <hostname> -p <port> -r <rate> "
    "-n <num packets> -q <seq_no> -l <length> -c <echo>\n");
    exit(1);
}

void invalidRange(char* option){
    fprintf(stderr, "Error: Invalid range value for %s\n", option);
    exit(1);
}

void setup_err(char* msg){
    fprintf(stderr, msg);
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
                //assuming rate must be greater than 0
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
                if(MAX_LEN <= *length) invalidRange("length");
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

    //arg values
    char* hostName = NULL;
    int port, rate, numPkts, echo;
    uint seq_no, len;
    getargs(&hostName, &port, &rate, &numPkts, &seq_no, &len, &echo, argc, argv);   

    //timespec struct for nanosleep to control rate
    struct timespec ts;

    //socket vars
    //s socket, socket address structs
    int s;
    struct sockaddr_in this_addr;
    struct sockaddr_in that_addr;
    socklen_t sockadd_sz = sizeof(that_addr);

    
    //number of bytes received on return from recvfrom()
    //+ buffer holding data received
    int rec_size;
    char buffer[MAX_LEN];

    //host entry
    struct hostent *he;


    fprintf(stdout, "hostName = %s\nport = %d\nrate = %d\nnum = %d\nseq = %u\nlen = %u\necho = %d\n",
            hostName, port, rate, numPkts, seq_no, len, echo);

    /*
     * calculate how long we need to sleep per packet in for loop
     */
    ts = calcSleepTime(rate);
    fprintf(stdout, "sleep rate is sec = %d, nano = %ld\n", (int) ts.tv_sec, ts.tv_nsec);


    /*
     * setup socket
     */
    if((s=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        setup_err("error creating socket\n");       
    }

    this_addr.sin_family = AF_INET;
    this_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this_addr.sin_port = htons(port);

    if(bind(s, (struct sockaddr *) &this_addr, sizeof(this_addr)) < 0){
        setup_err("error binding socket\n");
    }

    /*
     * get host addr
     */
    he = gethostbyname(host);
    if(he == NULL){
        setup_err("error finding host\n");
    }

    that_addr.sin_family = AF_INET;
    that_addr.sin_addr = hp->h_addr_list[0];
    that_addr.sin_port = htons(port);

    /*
     * for each packet, send
     */
    int i;
    for(i = 0; i < numPkts; i++){
        fprintf(stdout, "send a packet\n");
        nanosleep(&ts, NULL);
    }
    

    int x = sizeof(char);
    int y = sizeof(int);
    fprintf(stdout, "size of char = %d\n", x);
    fprintf(stdout, "size of int = %d\n", y);

    exit(0);

}
