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
int sockfd_receiver;
struct addrinfo hints_receiver, *servinfo_receiver, *p_receiver;
int rv_receiver;
struct sockaddr_storage their_addr_receiver;
socklen_t addr_len_receiver;
char s_receiver[INET6_ADDRSTRLEN];

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
 *	Receives the message from the sender
 *
 *	@param message:	Buffer where the received message is stored
 *	@return:	Number of bytes received
 */
int receiveMsg(msg * message)
{
	int numBytes = recvfrom(sockfd_receiver, message, sizeof(*message), 0, (struct sockaddr *)&their_addr_receiver, &addr_len_receiver);
	inet_ntop(their_addr_receiver.ss_family, get_in_addr((struct sockaddr *)&their_addr_receiver), s_receiver, sizeof s_receiver);
	return numBytes;
}

/**
 *	Sends an ACK message to the sender
 *
 *	@param hostname:	IP address of the host
 *	@param hostUDPport:	Port number of the host	
 *	@param ack_msg:		ACK message to be sent
 *	@return:		0 if message sent succesfully, 1 or 2 otherwise
 */
int sendACK(char * hostname, unsigned short int hostUDPport, ack ack_msg)
{
	char senderPort[50];
	sprintf(senderPort, "%d", hostUDPport);
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(hostname, senderPort, &hints, &servinfo)) != 0) {
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

	if ((numbytes = sendto(sockfd, &ack_msg, sizeof(ack_msg), 0,
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

	memset(&hints_receiver, 0, sizeof hints_receiver);
	hints_receiver.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints_receiver.ai_socktype = SOCK_DGRAM;
	hints_receiver.ai_flags = AI_PASSIVE; // use my IP

	if ((rv_receiver = getaddrinfo(NULL, myport, &hints_receiver, &servinfo_receiver)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_receiver));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p_receiver = servinfo_receiver; p_receiver != NULL; p_receiver = p_receiver->ai_next) {
		if ((sockfd_receiver = socket(p_receiver->ai_family, p_receiver->ai_socktype, p_receiver->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd_receiver, p_receiver->ai_addr, p_receiver->ai_addrlen) == -1) {
			close(sockfd_receiver);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p_receiver == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

    	freeaddrinfo(servinfo_receiver);
	
	return 0;
}

/**
 *	Receives data from the sender reliably
 *
 *	@param myUDPport:	Receiver's port number
 *	@param destinationFile:	File where the received data is to be written
 */
void reliablyReceive(unsigned short int myUDPport, char* destinationFile)
{
	if(initializeUDPListener(myUDPport) != 0)
	{
		perror("Error while initializing UDP listener");	
	}
}

int main(int argc, char** argv)
{
	unsigned short int udpPort;
	
	if(argc != 3)
	{
		fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
		exit(1);
	}
	
	udpPort = (unsigned short int)atoi(argv[1]);
	
	reliablyReceive(udpPort, argv[2]);
}
