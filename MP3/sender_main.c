#include <stdio.h>
#include <stdlib.h>

/**
 *	Extracts first bytesToTransfer from filename
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

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer)
{
	char * data = getFileContents(filename, bytesToTransfer);

	if(data)
	{

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
