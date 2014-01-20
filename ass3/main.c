#include "rtspd.h"


//// BEEEEEJJJ
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
   
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[])
{
    char* port;
  
    if(argc==2)
    {
        port = (argv[1]);
    }
    else{
        printf("Please provide a socket \n");
        return 1;
    }
  
  	// Inspired by BEEJ's tutorial server
  	
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    struct sigaction sa;
    int yes =1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    socklen_t sin_size;
    ssize_t result;
    const char *buffer[1024] = { 0 };
    char clientaddrport[1024];
    int connected = 1;
    srand(time(NULL));
  
  
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
  
    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
  
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
      
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
      
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
      
        break;
    }
  
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
  
    freeaddrinfo(servinfo); // all done with this structure
  
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
  
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
  
    printf("server: waiting for connections...\n");
    RTSPClient.currentState = STATE_INIT;
  
  	
  
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
      
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        int pid = fork();
        if(pid==0){
        	close(sockfd); // this is the fork / client handler
        	
            while(yes){
                char buf[1024];
                char *buffer = buf;
                int messRec = recv(new_fd, buffer, 1024, 0);
                if(messRec == -1)
                {
                    printf("Error receiving message\n");
                    exit(-1);
                }
                if(messRec == 0)
                {
                    printf("The client disconnected\n");
                    close(new_fd);
                    exit(0);
                }
                printf("\nRTSP Client request:\n");
                printf("%s\n", buffer);
                int len = strlen(buffer);
                
                // Parse the current response
                parseRTSPmessage(buffer,len);
                
                // init a buffer to hold response
                char response[1024];
                char* resp = response;
                struct send_frame_data data;
              
              	/*
              	 * SETUP REQUEST HANDLER
              	 */
                if(RTSPclientmsg.cmd == SETUP){
                    if(RTSPClient.currentState == STATE_READY || RTSPClient.currentState == STATE_PLAY || RTSPClient.currentState == STATE_PAUSE){
                        serverResponse(SETUP,455,response);
                    }
                    else{
                        RTSPClient.prevAction = 1;
                        RTSPClient.cSeq = RTSPclientmsg.cSeq;
						if(RTSPClient.session == NULL){
							RTSPClient.session = rand(); // Initialize the Session variable
						}
                        strcpy(RTSPClient.videoName, RTSPclientmsg.videoName);
                        CvCapture *movie = client_requested_file();
                        if(movie == NULL){
                            serverResponse(SETUP,404,response);
                			RTSPClient.currentState = STATE_INIT;
                        }
                        else{
                            serverResponse(SETUP,200,response);
                            RTSPClient.currentState = STATE_READY;
                			cvReleaseCapture(&movie);
                        }
                    }
                }
                /*
              	 * PLAY REQUEST HANDLER
              	 */
                else if(RTSPclientmsg.cmd == PLAY) {
                    if(RTSPClient.currentState == STATE_INIT){
                        serverResponse(PLAY,455,response);
                    }
                    else{
                        if(RTSPClient.currentState == STATE_READY){
                            CvCapture *movie = client_requested_file();
                            if(movie == NULL) {//try to see if the file their trying to open exists or not
                                RTSPClient.cSeq = RTSPclientmsg.cSeq;
                                serverResponse(PLAY, 455, response);
                            }
                            else{
                                RTSPClient.currentState = STATE_PLAY;
                                RTSPClient.prevAction = RTSPclientmsg.cmd;
                                RTSPClient.cSeq = RTSPclientmsg.cSeq;
                                serverResponse(PLAY, 200, response );
                              
                                data.vid = movie;
                                data.socket = new_fd;
                                data.scale = RTSPclientmsg.scale;
                                
                                struct sigevent play_event;
                                struct sigevent play_data;
                                struct itimerspec play_interval;
                                memset(&play_event, 0, sizeof(play_event));
                                
                                /*
                                 * Inspired by the code snippets in the Assignment description
                                 */
                                
                                play_event.sigev_notify = SIGEV_THREAD;
                                play_event.sigev_value.sival_ptr = &data;
                                play_event.sigev_notify_function = send_frame;
                              
                                play_interval.it_interval.tv_sec = 0;
                                play_interval.it_interval.tv_nsec = 40 * 1000000; // 40 ms in ns
                                play_interval.it_value.tv_sec = 0;
                                play_interval.it_value.tv_nsec = 1;
                              
                                timer_create(CLOCK_REALTIME, &play_event, &play_timer);
                                timer_settime(play_timer, 0, &play_interval, NULL);
                            }
                          
                        }
                        else{ // Need this to handle a change in Scale and resume from pause
                            RTSPClient.currentState = STATE_PLAY;
                            RTSPClient.prevAction = RTSPclientmsg.cmd;
                            RTSPClient.cSeq = RTSPclientmsg.cSeq;
                            serverResponse(PLAY, 200, response );
                          
                            data.scale = RTSPclientmsg.scale;
                            
                            struct sigevent play_event;
                            struct sigevent play_data;
                            struct itimerspec play_interval;
                            memset(&play_event, 0, sizeof(play_event));
                            
                            /*
                             * Inspired by the code snippets in the Assignment description
                             */
                            
                            play_event.sigev_notify = SIGEV_THREAD;
                            play_event.sigev_value.sival_ptr = &data;
                            play_event.sigev_notify_function = send_frame;
                          
                            play_interval.it_interval.tv_sec = 0;
                            play_interval.it_interval.tv_nsec = 40 * 1000000; // 40 ms in ns
                            play_interval.it_value.tv_sec = 0;
                            play_interval.it_value.tv_nsec = 1;
                          
                            // Resume timer or adjust play interval
                            timer_settime(play_timer, 0, &play_interval, NULL);
                          
                        }
                    }
                }
                /*
              	 * PAUSE REQUEST HANDLER
              	 */
                else if(RTSPClient.currentState == STATE_PLAY && RTSPclientmsg.cmd == PAUSE){
                    RTSPClient.currentState = STATE_PAUSE;
                    RTSPClient.prevAction = RTSPclientmsg.cmd;
                    RTSPClient.cSeq = RTSPclientmsg.cSeq;
                    serverResponse(PAUSE, 200, response);
                    stop_timer();
                }
                /*
              	 * TEARDOWN REQUEST HANDLER
              	 */
                else if( RTSPclientmsg.cmd == TEARDOWN ){
                    if(RTSPClient.currentState == STATE_INIT){
                        serverResponse(TEARDOWN,455,response);
                    }
                    else{
                        if(RTSPClient.currentState == STATE_PAUSE){
                            RTSPClient.currentState = STATE_INIT;
                            RTSPClient.prevAction = RTSPclientmsg.cmd;
                            RTSPClient.cSeq = RTSPclientmsg.cSeq;
                            serverResponse(TEARDOWN, 200, response );
                            // The following line is used to delete a timer.
                            stop_timer();
                            timer_delete(play_timer);
                            cvReleaseCapture(&data.vid);
                            connected = 0;
                        }
                        else{
                            if(RTSPClient.currentState == STATE_READY){
                                RTSPClient.currentState = STATE_INIT;
                                RTSPClient.prevAction = RTSPclientmsg.cmd;
                                RTSPClient.cSeq = RTSPclientmsg.cSeq;
                                serverResponse(TEARDOWN, 200, response );
                            }
                            else{
                                if(RTSPClient.currentState == STATE_PLAY) {
                                    RTSPClient.currentState = STATE_INIT;
                                    RTSPClient.prevAction = RTSPclientmsg.cmd;
                                    RTSPClient.cSeq = RTSPclientmsg.cSeq;
                                    serverResponse(TEARDOWN, 200, response );
                                    stop_timer();
                                    timer_delete(play_timer);
                                }
                              
                            }
                        }
                    }
                }
                /*
              	 * INVALID PAUSE REQUEST HANDLER
              	 */
                else if(RTSPclientmsg.cmd == PAUSE){
                    if(RTSPClient.currentState == STATE_INIT || RTSPClient.currentState == STATE_READY){
                        serverResponse(PAUSE,455,response);
                    }
                }
                
                /*
              	 * UNKNOWN REQUEST HANDLER
              	 */
                else{
                    serverResponse(0, 501, response);
                    printf("\n%s\n", response);
                }
                
                // Send response and print to console
                send(new_fd,resp,strlen(resp),0);
                printf("\nResponse sent to client:\n\n%s", response);
                memset(&buffer[0], 0, sizeof(buffer));
            }
        }
        else { // This is the listener. No need for the new fd.
            if (pid == -1){ // Fork error.
                perror("fork");
                return 1;
            }
            else{
                close(new_fd);
            }
        }
        // Client handler must close now.
        printf("\nThis socket has been closed.\n");
        close (new_fd);
    }
    return 0;
}
