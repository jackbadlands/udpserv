#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>



#ifndef WIN32

#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#else

#define WINVER 0x0501
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <fcntl.h>


#endif

#include <errno.h>
#include <signal.h>

#define BUFSIZE 4096

#include "popen_arr.h"

int s_recv = -1;
    
unsigned char buf[BUFSIZE];
    
int main(int argc, char* argv[]) {
    if (argc>=2 && !strcmp(argv[1], "--version")) {
        printf("udpserv 0.0\n");
        return 0;
    }
    if (argc<5 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-?")) {
        fprintf (stderr, 
        "simple UDP server executing arbitrary program per each UDP packet\n"
        "like \"socat udp-l:5555,fork exec:something\"\n"
        "Buffer size (maximum input and output sizes) is %d bytes\n"
        "IPv6 is supported when you use respective recv_addr\n"
        "Intended use case: accessing network statistics and/or establishing mosh session\n"
        "\twhen packet loss is so big that TCP can't go thought.\n"
        "\n"
        "Implemented by Vitaly _Vi Shukela in 2013, License=MIT\n"
        "\n"
        "Usage:\n"
        "\tudpserv delay_after_each_request_microsec recv_host recv_port cmdline...\n"
        "Example:\n"
        "\tudpserv 25000 0.0.0.0 6767 sh -c 'read I; ifconfig \"$I\"' \n"
        "\techo eth0 | nc -u 127.0.0.1 5555 -q 1\n"
        "\tPATH=/..../mosh.git/src/frontend:$PATH ./udpserv 250000 0.0.0.0 2222 ./mosh-server-script 0x09EA92A2DB6D6082 0x09EA92A2DB6D6082\n"
        ,BUFSIZE);
        return 1;
    }
    
    struct addrinfo hints;
    struct addrinfo *recv_addr;
    struct addrinfo *send_addr;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;
    
    int usleep_interval = atoi(argv[1]);
    char* recv_host = argv[2];
    char* recv_port = argv[3];
    
    if (!strcmp(recv_host, "NULL")) recv_host=NULL;
    if (!strcmp(recv_port, "NULL")) recv_port=NULL; 
    
    
    int gai_error;
    gai_error=getaddrinfo(recv_host,recv_port, &hints, &recv_addr);
    if (gai_error) { fprintf(stderr, "getaddrinfo 1: %s\n",gai_strerror(gai_error)); return 4; }
    if (!recv_addr) { fprintf(stderr, "getaddrinfo returned no addresses\n");   return 6;  }
    if (recv_addr->ai_next) {
        fprintf(stderr, "Warning: using only one of addresses retuned by getaddrinfo\n");
    }
    
    gai_error=getaddrinfo(recv_host,recv_port, &hints, &send_addr); /* Just allocate the necessary data */
    
    s_recv = socket(recv_addr->ai_family, recv_addr->ai_socktype, recv_addr->ai_protocol);
    if (s_recv == -1) { perror("socket"); return 7; }
    {
        int one = 1;
        setsockopt(s_recv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    }
    if (bind(s_recv, recv_addr->ai_addr, recv_addr->ai_addrlen)) {  perror("bind");  return 5;  }
    
    signal(SIGCHLD, SIG_IGN); // A charm agains zombies
    
    for (;;) {
        int ret = recvfrom(s_recv, buf, sizeof buf, 0, send_addr->ai_addr, &send_addr->ai_addrlen);
        if (ret >= 0) {
            if(!fork()) {
                FILE *in, *out;
                popen2_arr_p(&in, &out,  argv[4], (const char**)argv+4, NULL);
                if(in) {
                    fwrite(buf, 1, ret, in);
                    fclose(in);
                }
                if(out) {
                    ret = fread(buf, 1, sizeof buf, out);
                    fclose(out);
                }
                sendto(s_recv, buf, ret, 0,  send_addr->ai_addr,  send_addr->ai_addrlen);
                _exit(0);
            }
        }
        
        {
            int ret;
            struct timespec req = {usleep_interval/1000000,(usleep_interval%1000000)*1000}, rem;
            for(;;) {
                ret = nanosleep(&req, &rem);
                if (ret == -1 && (errno == EINTR || errno==EAGAIN)) {
                    req = rem;
                    continue;
                }
                break;
            }
        }
    }
}
