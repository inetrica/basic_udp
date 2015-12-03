#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_LEN 50010
#define NANO 1000000000
#define MIN_CHAR 33
#define MAX_CHAR 126

void 
usage() {
    fprintf(stderr, "Usage: blaster -s <hostname> -p <port> -r <rate> "
    "-n <num packets> -q <seq_no> -l <length> -c <echo>\n");
    exit(1);
}

void 
invalidRange(char* option){
    fprintf(stderr, "Error: Invalid range value for %s\n", option);
    exit(1);
}

void 
err(char* msg){
    fprintf(stderr, msg);
    exit(1);
}

unsigned int 
getUint(char* option, char* arg){
    unsigned long tmp = 0;
    errno = 0;
    tmp = strtoul(arg, NULL, 10);
    if(errno != 0 || tmp > UINT_MAX){
        invalidRange(option);
    }

    return (uint) tmp;

}

void 
getargs(char **hostname, int *port, int *rate, int *num, 
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
struct timespec 
calcSleepTime(int rate){
    //const uint nsps = NANO; //1000000000 nanoseconds per second
    struct timespec tspec;
    long nanosecSleep = NANO/rate;
    int sec = nanosecSleep/NANO;
    tspec.tv_sec = sec;
    tspec.tv_nsec = nanosecSleep % NANO;
    return tspec;
}

struct timespec
subtract(struct timespec a, struct timespec b){
    struct timespec tmp;
    //if time between nanosecs is < 0, need to
    //take time away from secs, add back to nsec
    if((b.tv_nsec - a.tv_nsec) < 0){
        tmp.tv_sec = (b.tv_sec - a.tv_sec) - 1;
        tmp.tv_nsec = (b.tv_nsec - a.tv_nsec) + NANO;
    } else {
        tmp.tv_sec = (b.tv_sec - a.tv_sec);
        tmp.tv_nsec = (b.tv_nsec - a.tv_nsec);
    }
    return tmp;
}

void 
createPacket(uint seq, uint len, char data[], int type){
    char pktType;
    int pos = 0;
    switch(type){
    case 0:
        pktType = 'D';
        break;
    case 1:
        pktType = 'E';
        break;
    case 2:
        pktType = 'C';
        break;
    default:
        err("Invalid packet type while trying to create packet");
    }

    //indicate packet type
    //fprintf(stdout, "size of pktType char = %d\n", (int) sizeof(pktType));
    *(data) = pktType;
    //fprintf(stdout, "seq %u and len %u\n", seq, len);
    
    if(sizeof(seq) != 4 || sizeof(len) != 4){
        err("sizeof(uint) is not 4?\n");
    }
    memcpy(data+1, &seq, sizeof(seq));
    memcpy(data+5, &len, sizeof(len));

    int char_diff = MAX_CHAR - MIN_CHAR;

    int i;
    pos = 9;
    for(i = 0; i < len; i++){
        int dval = (seq + i) % char_diff;
        data[pos] = (char) (MIN_CHAR + dval);
        pos++;
    }

    if(pktType == 'D'){
        fprintf(stdout, "seq no: %u\ndata: ", seq);
        for(i = 9; i < 13; i++){
            fprintf(stdout, "%c", data[i]);
        }
        fprintf(stdout, "\n\n");
    }

    data[pos] = '\0';
    return;

}

void
decodeEcho(char packet[]){
    //make sure packet received was an echo...
    if(packet[0] != 'C'){
        fprintf(stdout, "received a non-echo packet\n");
        return;
    }
    uint seq, len;
    char *data = NULL;
    int i = 0;
    
    memcpy(&seq, packet + 1, sizeof(seq));
    memcpy(&len, packet + 5, sizeof(len));

    data = (packet + 9);

    fprintf(stdout, "echo: %u\nedata: ", seq);
    for(i = 9; i < 13; i++){
        fprintf(stdout, "%c", packet[i]);
    }
    fprintf(stdout, "\n\n");

}

int 
main(int argc, char *argv[]){

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
    char recvBuff[MAX_LEN];

    //host entry
    struct hostent *he;


    /*
    fprintf(stdout, "hostName = %s\nport = %d\nrate = %d\nnum = %d\nseq = %u\nlen = %u\necho = %d\n",
            hostName, port, rate, numPkts, seq_no, len, echo);
            */

    /*
     * calculate how long we need to sleep per packet in for loop
     */
    ts = calcSleepTime(rate);
    //fprintf(stdout, "sleep rate is sec = %d, nano = %ld\n", (int) ts.tv_sec, ts.tv_nsec);


    /*
     * setup socket
     */
    if((s=socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        err("error creating socket\n");       
    }

    this_addr.sin_family = AF_INET;
    this_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this_addr.sin_port = htons(0);

    if(echo && bind(s, (struct sockaddr *) &this_addr, sizeof(this_addr)) < 0){
        err("error binding socket\n");
    }

    /*
     * get host addr
     */
    he = gethostbyname(hostName);
    if(he == NULL){
        err("error finding host\n");
    }

    that_addr.sin_family = AF_INET;
    //that_addr.sin_addr = he->h_addr_list[0];
    memcpy(&(that_addr.sin_addr), *(he->h_addr_list), he->h_length);
    that_addr.sin_port = htons(port);

    

    /*
     * for each packet, send
     */
    struct timespec start;
    struct timespec end;
    int i, type;
    //use fd_set and timeout to recv if need echo
    fd_set sockets;
    struct timeval to;
    int sel;

    type = 0;
    for(i = 0; i < numPkts + 1; i++){
        if(echo){
            to.tv_sec = ts.tv_sec;
            to.tv_usec = ts.tv_nsec/1000;
            FD_ZERO(&sockets);
            FD_SET(s, &sockets);
        }
        if(i == numPkts) type = 1;//end packet
        //create packet
        createPacket(seq_no, len, buffer, type);
        //fprintf(stdout, "have packet\n%c %u %u %s\n", buffer[0], (uint) buffer[1], (uint) buffer[5], buffer+9);
        //send the packet
        clock_gettime(CLOCK_REALTIME, &start);
        if(sendto(s, buffer, len, 0, 
                    (struct sockaddr *) &that_addr, sockadd_sz) < 0){
            err("error sending packet\n");   
        }
        
        sel = -1;
        struct timespec tleft;
        if(echo && type == 0 && (sel = select(FD_SETSIZE, &sockets, NULL, NULL, &to)) > 0){
            

            rec_size = recvfrom(s, recvBuff, MAX_LEN, 0, (struct sockaddr *) &that_addr, &sockadd_sz);
            if(rec_size > 0){
                recvBuff[rec_size] = '\0';
                decodeEcho(recvBuff);
            }
            //determine time left to sleep
            tleft.tv_sec = to.tv_sec;
            tleft.tv_nsec = to.tv_usec*1000;
            nanosleep(&tleft, NULL);
        } else {
            if(sel != 0){
                clock_gettime(CLOCK_REALTIME, &end);
                struct timespec diff = subtract(start, end);
                tleft = subtract(diff, ts);

                //sleep to match rate
                if((tleft.tv_sec > 0) || (tleft.tv_sec == 0 && tleft.tv_nsec > 0)){
                    nanosleep(&tleft, NULL);
                }
            }
        }

        seq_no += len;

    }
    

    exit(0);

}
