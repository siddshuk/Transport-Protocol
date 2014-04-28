#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFERSIZE 1400

/*
 *  Global Variables
 */
char myport[50];
int sockfd_sender;
struct addrinfo hints_sender, *servinfo_sender, *p_sender;
int rv_sender;
struct sockaddr_storage their_addr_sender;
socklen_t addr_len_sender;
char s_sender[INET6_ADDRSTRLEN];

/*
 * 	User defined structs
 */
typedef struct ack {
	int seq_num_request;	
} ack;

typedef struct msg {
	char messages[BUFFERSIZE];
	int seq_num;
	int final;
} msg;

/**
 *	Get sockaddr, IPv4 or IPv6
 *
 *	@param sa:	sockaddr struct
 *	@return:	IPv4 or IPv6 address
 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 *	Extracts first bytesToTransfer from filename
 *	
 *	@param filename:	Name of file to be read
 *	@param bytesToTransfer:	Number of bytes to be read from file
 *	@return:		buffer with the contents of the file
 */
char * getFileContents(char * filename, unsigned long long int bytesToTransfer)
{
	char * buffer = 0;
	FILE * file = fopen(filename, "rb");

	if(file)
	{
		fseek(file, 0, SEEK_SET);
		buffer = malloc(bytesToTransfer);
		if(buffer)
		{
			fread(buffer, 1, bytesToTransfer, file);
		}
		fclose(file);
	}

	return buffer;
}

/**
 *	Receives the ACK message from sender
 *
 *	@param ack_msg:	Buffer where the received ACK is stored
 *	@return:	Number of bytes received
 */
int receiveACK(ack * ack_msg)
{
	int numBytes = recvfrom(sockfd_sender, ack_msg, sizeof(*ack_msg), 0, (struct sockaddr *)&their_addr_sender, &addr_len_sender);
	return numBytes;
}

/**
 *	Starts accepting all ACK messages from sender one by one
 *
 *	@param arg:	NULL argument required by pthread
 *	@return:	NULL
 */
void * acceptACK(void * arg)
{
	while(1)
	{
		ack ack_msg;
		receiveACK(&ack_msg);		
	}

	return NULL;
}

/**
 *	Sends a message to the receiver
 *	
 *	@param hostname:	IP address of the host
 *	@param hostUDPport:	Port number of the host
 *	@param message:		Message to be sent
 *	@return:		0 if successful, 1 or 2 otherwise
 */
int sendMsg(char * hostname, unsigned short int hostUDPport, msg message)
{
	char receiverPort[50];
	sprintf(receiverPort, "%d", hostUDPport);
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(hostname, receiverPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to bind socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, &message, sizeof(message), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	close(sockfd);

	return 0;

}

/**
 *	Initialize the UDP listener connection that is, bind to socket
 *
 *	@param port: 	Port number to be binded to
 *	@return:	0 if initialization successful, 1 or 2 otherwise
 */
int initializeUDPListener(unsigned short int port)
{
	sprintf(myport, "%d", port);

	memset(&hints_sender, 0, sizeof hints_sender);
	hints_sender.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints_sender.ai_socktype = SOCK_DGRAM;
	hints_sender.ai_flags = AI_PASSIVE; // use my IP

	if ((rv_sender = getaddrinfo(NULL, myport, &hints_sender, &servinfo_sender)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_sender));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p_sender = servinfo_sender; p_sender != NULL; p_sender = p_sender->ai_next) {
		if ((sockfd_sender = socket(p_sender->ai_family, p_sender->ai_socktype,
				p_sender->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd_sender, p_sender->ai_addr, p_sender->ai_addrlen) == -1) {
			close(sockfd_sender);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p_sender == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

    	freeaddrinfo(servinfo_sender);
	
	return 0;
}

/**
 *	Sends data to the receiver reliably
 *
 *	@param hostname:	IP address of the host
 *	@param hostUDPport:	Port number of the host
 *	@param filename:	Name of file to be read
 *	@param bytesToTransfer:	Number of bytes to be read from file	
 */
void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer)
{
	char * data = getFileContents(filename, bytesToTransfer);

	if(data)
	{
		if(initializeUDPListener(hostUDPport - 1) != 0)
		{
			perror("Error while initializing UDP listener");	
		}
	}
	else
	{
		perror("Error while reading contents of the given file\n");
	}
}

int main(int argc, char** argv)
{
	unsigned short int udpPort;
	unsigned long long int numBytes;
	
	if(argc != 5)
	{
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	
	udpPort = (unsigned short int)atoi(argv[2]);
	numBytes = atoll(argv[4]);

	reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
} 
