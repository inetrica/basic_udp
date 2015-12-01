#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define MAX_LEN 50010
#define BILLION 1000000000

void usage() {
    fprintf(stderr, "Usage: blastee -p <port> -c <echo>\n");
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

struct summary {
    uint num;
    uint totalBytes;
};

void getargs(int *port, int *echo, int argc, char *argv[]){
    int a = 0;
    int c = 0;
    while((a = getopt(argc, argv, "p:c:")) != -1){
        c++;
        switch(a){
            case 'p':
                *port = atoi(optarg);
                if(*port <= 1024 || 65536 <= *port) invalidRange("port");
                break;
            case 'c':
                *echo = atoi(optarg);
                if(*echo != 0 && *echo != 1) invalidRange("echo");
                break;
            default:
                usage();
        }
    }
    if(c != 2) usage();

}

//returns 0 if Data packet, 1 if End packet, -1 if something else
int decodePrint(char packet[], int port){
    //0 = data
    //1 = seq no
    //5 = len
    //9 = data
    char type;
    uint seq, len;
    char *data = NULL;
    type = packet[0];
    
    memcpy(&seq, packet + 1, sizeof(seq));
    memcpy(&len, packet + 5, sizeof(len));

    data = (packet + 9);
    /*if(strlen(data) != len){
        fprintf(stdout, "string lengths don't match\n");
    }*/
    fprintf(stdout, "ip addr\n%d\n%u\n%u\ntime\n", port, len, seq);

    //print first 4 data chars
    int i;
    for(i = 0; i < 4; i++){
        fprintf(stdout, "%c", packet[9+i]);
    } 
    fprintf(stdout, "\n\n");


    switch(type){
    case 'D':
        return 0;
    case 'E':
        return 1;
    default:
        return -1;
    }

}

struct timespec
subtract(struct timespec a, struct timespec b){
    struct timespec tmp;
    //if time between nanosecs is < 0, need to
    //take time away from secs, add back to nsec
    if((b.tv_nsec - a.tv_nsec) < 0){
        //carry over
        tmp.tv_sec = (b.tv_sec - a.tv_sec) - 1;
        tmp.tv_nsec = (b.tv_nsec - a.tv_nsec) + BILLION;
    } else {
        tmp.tv_sec = (b.tv_sec - a.tv_sec);
        tmp.tv_nsec = (b.tv_nsec - a.tv_nsec);
    }
    return tmp;
}

double
timeInSec(struct timespec ts){
    double secn = ts.tv_nsec/(BILLION*1.0);
    return ts.tv_sec + secn;
}

void printSummary(struct summary s, struct timespec ts){
    double duration = timeInSec(ts);
    double avgpps = s.num/duration;
    double avgbps = s.totalBytes/duration;
    fprintf(stdout, "%u packets\n%u bytes\n%G Packets/sec\n%G Bytes/sec\nduration: %G\n", s.num, s.totalBytes, avgpps, avgbps, duration);
}

int main(int argc, char *argv[]){

    //port number, echo bool, socket s
    int port, echo;
    int s;

    //stockaddr_in struct for bind this
    //sockaddr_in struct for receiving from remote
    //socklen_t for recvfrom()
    struct sockaddr_in this_addr;
    struct sockaddr_in that_addr;
    socklen_t sockadd_sz = sizeof(that_addr);

    //number of bytes received on return from recvfrom()
    //+ buffer holding data received
    int rec_size;
    char buffer[MAX_LEN];

    //determine time spent receiving
    int startedReceiving = 0;
    struct timespec first;
    struct timespec last;


    getargs(&port,&echo, argc, argv);   

    fprintf(stdout, "port = %d\necho = %d\n",
            port, echo);



    //create socket
    if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        setup_err("Error creating socket\n");
    }
    

    //bind socket
    this_addr.sin_family = AF_INET;
    this_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this_addr.sin_port = htons(port);

    if(bind(s, (struct sockaddr *) &this_addr, sizeof(this_addr)) < 0){
        setup_err("Error binding socket\n");
    }

    struct summary summ;
    summ.num = 0;
    summ.totalBytes = 0;
    while(1){
        rec_size = recvfrom(s, buffer, MAX_LEN, 0, 
                (struct sockaddr *) &that_addr, &sockadd_sz);
        
        fprintf(stdout, "rec_size = %d\n", rec_size);
        if(rec_size > 0){

            if(startedReceiving == 0){
                clock_gettime(CLOCK_MONOTONIC, &first);
                startedReceiving = 1;
            }
            summ.num++;
            summ.totalBytes += rec_size;

            buffer[rec_size] = '\0';//append null
            //fprintf(stdout, "received \"%s\"\n", buffer);
            if(decodePrint(buffer, port) > 0){
                break;
            }
        }
    }


    clock_gettime(CLOCK_MONOTONIC, &last);
    struct timespec recvTime = subtract(first, last);

    printSummary(summ, recvTime);


    exit(0);

}
