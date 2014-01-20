#include "rtspd.h"

// msg -> buffer to load RTSP message from client
// len -> length of the message
// Extracts command from client message and calls parseRTSPcmd()
void parseRTSPmessage(char* msg,int len)
{
    char buf[1024];
    char* temp = buf;
    strcpy(temp, msg);
   
    while(*temp == "") temp++;
   
    // Because pointer: only first character available
    switch(*temp) {
        case 'S':
            *(temp+5)='\0';// Add null character to show EOS
            if(!strcmp(temp+1, "ETUP")){
                RTSPclientmsg.cmd = SETUP;
                parseRTSPcmd(msg);
            }
            break;
        case 'P':  // Read in the next 3 chars to figure out if play or pause
            *(temp+4)='\0'; // Add null character to show EOS
            if(!strcmp (temp+1,"LAY")) {
                RTSPclientmsg.cmd = PLAY;
                parseRTSPcmd(msg);
            }
            else if(!strcmp (temp+1,"AUS")){
                RTSPclientmsg.cmd = PAUSE;
                parseRTSPcmd(msg);
            }
            break;
        case 'T':
            *(temp+8)='\0'; // Add null character to show EOS
            if(!strcmp(temp+1,"EARDOWN")){
                RTSPclientmsg.cmd = TEARDOWN;
                parseRTSPcmd(msg);
            }
            break;
        default:
            break;
    }
}


// cmd -> client request
// Parse header and deal with ASCII chars
void parseRTSPcmd(char* cmd)
{

	char headerBuffer[1024];
	char hBuff[1024];
	char* header_content = headerBuffer;
	char* hd = hBuff;

	hd = "client_port=";
	parse_request_headers(cmd,hd, header_content);
	if(header_content != 0)
	{
		//convert char* to int
		RTSPclientmsg.port = atoi(header_content);
	}
	hd = "Session";
	parse_request_headers(cmd,hd,header_content);
	if(header_content != 0)
	{
		RTSPclientmsg.session = atoi(header_content);
	}

	// GET VIDEO NAME
	char *token = (char*) malloc(25);
	char *rest;
	char cmd2[1024];
	char *cmdtemp = cmd2;
	strcpy(cmdtemp, cmd);
	token = strtok_r(cmd, " ", &rest);
	token = strtok_r(NULL, " ", &rest);
	strcpy(RTSPclientmsg.videoName, token);


	// GET CSEQ 
	// Parse until token is pointed at CSEQ
	token = strtok_r(NULL, "\n", &rest);
	token = strtok_r(NULL, "\n", &rest);
	token = strtok_r(token, " ", &rest);
	token = strtok_r(NULL, " ", &rest);
	RTSPclientmsg.cSeq = atoi(token);

   	// GET SCALE/SPEED
   	// Parse until token at scale
	if(RTSPclientmsg.cmd == PLAY){
		token = strtok_r(cmdtemp, "\n", &rest);
		token = strtok_r(NULL, "\n", &rest);
		token = strtok_r(NULL, "\n", &rest);
		token = strtok_r(token, " ", &rest);
		token = strtok_r(NULL, " ", &rest);
		RTSPclientmsg.scale = atoi(token);
	}

}

// msg	-> client message
// hd	-> header
// hdcontent ->	finalized vals used for parseRTSPcmd
void parse_request_headers( char* msg, char* hd, char* hdcontent )
{
	char tmparray[1024];
	char rnarray[3];
	char headerarray[ strlen( hd )+1 ];
	char* tmp = tmparray;
	char* rn =  rnarray;
	char* header = headerarray;
	char cmp[] = "\r\n";

	strcpy( tmp, msg );
	int i;
	int len = strlen( msg );
	for( i = 0; i < len; i++ )
	{
		strncpy( rn, tmp, 2 );
		rn[2] = '\0';
		while( strcmp( rn, cmp ) && i < len )
		{
			// iterate till end of line
			tmp++; 
			i++;
			strncpy( rn, tmp, 2 );
			rn[2] = '\0';
		}
		tmp += 2;
		i += 2;
		while( *tmp == ' ' )
		{
			tmp++; // eliminate white space in the string
			i++;
		}

		strncpy( header, tmp, strlen(hd) );
		header[ strlen(hd) ] = '\0';
		if( !strcmp( header, hd ) )
		{
			break;
		}
	}
	if( i >= len )
	{
		hdcontent = 0;
		return;
	}
   
	tmp += strlen( hd );
	while( *tmp == ' ' || *tmp == ':' ) 
	{
		tmp++;
	}
	i = 0;
	while( tmp[i] != '\r' ) i++;
	tmp[ i ] = '\0';
	strcpy( hdcontent, tmp );
   
}


// cmd	-> Command issued by client (SETUP || PLAY || PAUSE || TEARDOWN)
// code	-> Resposne code (200 || 404)
// response	-> buffer to load resposne to be sent to client
// Send resposne to client
void serverResponse(int cmd, int code, char* response)
{
	// If not found
	if( code == 404 ){
		if( RTSPClient.session == 0 )
		{
			sprintf( response, "RTSP/1.0 404 Not Found\r\nCSeq: %d\r\n\r\n", RTSPClient.cSeq );
		}
		else
		{
			sprintf( response, "RTSP/1.0 404 Not Found\r\nCSeq: %d\r\nSession: %d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
		}
	}
	// If Good
	else if( code == 200 )
	{
		switch( cmd )
		{
			case SETUP:
				sprintf( response, "RTSP/1.0 200 OK\r\nCSeq: %d\r\nSession: %d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
				break;
			case PLAY:
				sprintf( response, "RTSP/1.0 200 OK\r\nCSeq: %d\r\nSession: %d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
			case PAUSE:
				sprintf( response, "RTSP/1.0 200 OK\r\nCSeq: %d\r\nSession: %d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
			case TEARDOWN:
				sprintf( response, "RTSP/1.0 200 OK\r\nCSeq: %d\r\nSession: %d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
				break;
			default:
				if(RTSPClient.session == 0)
				{
					sprintf( response, "RTSP/1.0 501 Not Implemented\r\nCSeq: %d\r\n\r\n", RTSPClient.cSeq );
				}
				else
				{
					sprintf( response, "RTSP/1.0 501 Not Implemented\r\nCSeq: %d\r\nSession:%d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
				}
				break;
		}
	}
	// If invalid
	else if( code == 455 )
	{
		sprintf( response, "RTSP/1.0 455 Invalid State \r\nCSeq: %d\r\nSession: %d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
	}
	else{
		if(  RTSPClient.session == 0 )
		{
			sprintf( response, "RTSP/1.0 501 Not Implemented\r\nCSeq: %d\r\n\r\n", RTSPClient.cSeq );
		}
		else
		{
			sprintf( response, "RTSP/1.0 501 Not Implemented\r\nCSeq: %d\r\nSession:%d\r\n\r\n", RTSPClient.cSeq, RTSPClient.session );
		}
	}
}

// Function to test if video can be opened
// if returns null; cannot be opened
CvCapture* client_requested_file()
{
    CvCapture *movie;
    movie = cvCaptureFromFile(RTSPClient.videoName);
    return movie;
}

// Fuction to be called by timer
void send_frame(union sigval sv_data)
{
	IplImage *image;
	CvMat *thumb;
	CvMat *encoded;
   
	struct send_frame_data *data = (struct send_frame_data *) sv_data.sival_ptr;

	// Obtain the next frame from the video file
	data->frame_num++;
	image = cvQueryFrame(data->vid);
	while (data->frame_num % data->scale !=0)
	{
		image = cvQueryFrame(data->vid);
		data->frame_num++;
	}
	if (image == NULL) 
	{
		// Close the video file
		cvReleaseCapture(&data->vid);
		stop_timer();
		return;
	   
	}

	int WIDTH = 300;
	int HEIGHT = 300;

	// convert to appropriate widthxheight
	thumb = cvCreateMat(WIDTH, HEIGHT, CV_8UC3);
	cvResize(image, thumb, CV_INTER_AREA);

	// Encode frame to jpeg : quality=50%.
	const static int encodeParams[] = { CV_IMWRITE_JPEG_QUALITY, 50 };
	encoded = cvEncodeImage(".jpeg", thumb, encodeParams);
	// After the call above, the encoded data is in encoded->data.ptr
	// and has a length of encoded->cols bytes.

	int jpeg_size = encoded->cols;
	int rtp_header_size = 12;
	char rtp_buffer[jpeg_size + rtp_header_size + 4];
	int rtp_pk_size = rtp_header_size + jpeg_size;
	int ts = data->scale * 40;

	// Setup RTP buffer
	rtp_buffer[0] = '$';     
	rtp_buffer[1] = 0;      
	rtp_buffer[2] = (rtp_pk_size & 0x0000FF00) >> 8;     
	rtp_buffer[3] = (rtp_pk_size & 0x000000FF);
	rtp_buffer[4] = 0x80;          
	rtp_buffer[5] = 0x9a;         
	rtp_buffer[7]  = data->frame_num & 0x0FF;          
	rtp_buffer[6]  = data->frame_num >> 8;
	rtp_buffer[11] = (ts & 0x000000FF);
	rtp_buffer[10] = (ts & 0x0000FF00) >> 8;
	rtp_buffer[9]  = (ts & 0x00FF0000) >> 16;
	rtp_buffer[8]  = (ts & 0xFF000000) >> 24;
	rtp_buffer[12] = 0x00;                           
	rtp_buffer[13] = 0x00;                              
	rtp_buffer[14] = 0x00;
	rtp_buffer[15] = 0x00;

	// Copy the image data into the rtp_buffer
	memcpy(&rtp_buffer[16],encoded->data.ptr, encoded->cols);

	//Send Everyting
	// +4 for header and jpg data
	send(data->socket, rtp_buffer,rtp_pk_size + 4, 0);
}

// From code snippets
// stop timer (PAUSE)
void stop_timer(void) {
   
    struct itimerspec play_interval;
   
    play_interval.it_interval.tv_sec = 0;
    play_interval.it_interval.tv_nsec = 0;
    play_interval.it_value.tv_sec = 0;
    play_interval.it_value.tv_nsec = 0;
    timer_settime(play_timer, 0, &play_interval, NULL);
   
}
