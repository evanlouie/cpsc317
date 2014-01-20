#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <cv.h>
#include <highgui.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#define BACKLOG 10    // how many pending connections queue will hold
#define STATE_INIT 0
#define STATE_READY 1
#define STATE_PLAY 2
#define STATE_PAUSE 3
#define SETUP 0
#define PLAY 1
#define PAUSE 2
#define TEARDOWN 4

timer_t play_timer;

// Function defintions

void parseRTSPmessage(char* msg,int len);
void parseRTSPcmd(char* cmd);
void parse_request_headers( char* msg, char* hd, char* hdcontent );
void serverResponse(int cmd, int code, char* response);
CvCapture* client_requested_file();
void send_frame(union sigval sv_data);
void stop_timer(void);

// Struct definitions

struct RTSPClient {
    struct sockaddr_in client_addr;
    int fileDefinition;
    int RTPport;
    int currentState;// READY, PLAY
    int prevAction; // SETUP, PLAY, etc.
    int session;
    int cSeq;
    int scale;
    char videoName[999];
} RTSPClient;

struct RTSPclientmsg{
    int cmd; // the defined value of SETUP, etc.
    int session;
    int cSeq;
    int port; // get the RTP port number from SETUP command
    int err; // 0: no err. else: err occured
    int scale;
    char videoName[999];
    char Connection[ 1024 ];
    char Proxy_Require[ 1024 ];
    char Transport[ 1024 ];
    char Require[ 1024 ];
} RTSPclientmsg;


struct RTSPservermsg{
    int session;
    int cSeq;
    int err;
} RTSPservermsg;

// Information struct for timer
struct send_frame_data {
    CvCapture *vid;
    int socket;
    int scale;
    int frame_num;
};
