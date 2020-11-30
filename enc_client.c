// encrypt client

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// max size that data can send and receive
#define MAX 70001

// update send data
void updateSend(char * buffer, char* temp, int index){
	int i,j = 0;
	for(i = index; i < strlen(buffer); i++){
		temp[j] = buffer[i];
		j++;
	}
}


// check if any bad characters exists in the array
bool checkBadChars(char *array, char *alphabet){
	int i, j, count;
	bool bad = false;
	char * p;
	for(i = 0; i < strlen(array); i++){
		for( j = 0; j < strlen(alphabet); j++){
			// if it is a good character then add one to the total count of good characters
			if(array[i] == alphabet[j]){
				count++;					
			}
		}
	}
	
	// if total count of good character is less than the total count of characters in the array, then there is at least one bad character
	if(count < strlen(array)-1){
		bad = true;
	}
	//printf("%i\n",bad);
	return bad;		
}

// Error function used for reporting issues
void error(const char *msg, int value) { 
	perror(msg); 
	exit(value); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname){
 
  	// Clear out the address struct
	memset((char*) address, '\0', sizeof(*address)); 

  	// The address should be network capable
  	address->sin_family = AF_INET;
  	// Store the port number
  	address->sin_port = htons(portNumber);
	//printf("port in client: %i\n", ntohs(address->sin_port));
  	// Get the DNS entry for this host name
  	struct hostent* hostInfo = gethostbyname(hostname); 
  	if (hostInfo == NULL) { 
    		error("ENC_CLIENT: ERROR, no such host\n", 2); 
	}
  	// Copy the first IP address from the DNS entry to sin_addr.s_addr
  	memcpy((char*) &address->sin_addr.s_addr, 
        	hostInfo->h_addr_list[0],
        	hostInfo->h_length);
}



int main(int argc, char *argv[]) {

	int socketFD, portNumber, textWritten, keyWritten, cipherRead, greetingRead;
  	struct sockaddr_in serverAddress;
  	char cipherBuffer[MAX], buffer[50], largeBuffer[MAX], smallBuffer[1000];
	char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int i;
	
  	// Check usage & args
  	if (argc < 4) { 
    		fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    		exit(1); 
  	}

	// open plain text file
	char plainPath[100];
	memset(plainPath, '\0', sizeof(plainPath));
	strcpy(plainPath, "./");
	strcat(plainPath, argv[1]);
	int plainFile = open(plainPath, O_RDONLY, 0777);
	if(plainFile == -1){
		fprintf(stderr, "ENC_CLIENT: Failed to open %s\n", argv[1]);
		exit(1);
	}	 

	// open key file
	char keyPath[100];
	memset(keyPath, '\0', sizeof(keyPath));
	strcpy(keyPath,"./");
	strcat(keyPath, argv[2]);
	int keyFile = open(keyPath, O_RDONLY, 0777);
	if(keyFile == -1){
		fprintf(stderr,"ENC_CLIENT: Failed to open %s\n", argv[2]);
		exit(1);
	}
	
	
	// read key and plaintext
	char * textBuffer = (char*)malloc(MAX* sizeof(char));
	char * keyBuffer = (char*)malloc(MAX*sizeof(char));
	memset(keyBuffer, '\0', sizeof(keyBuffer));
	memset(textBuffer, '\0', sizeof(textBuffer));
	size_t nreadp;
	size_t nreadk;

	// read data from files
	nreadp = read(plainFile, textBuffer, MAX);
	nreadk = read(keyFile, keyBuffer, MAX);
	
	// check size of key and plaintext
	if(strlen(keyBuffer) < strlen(textBuffer)){
		error("ENC_CLIENT:key size too short for the plaintext size", 1);
	}	

	// check for bad characters in key and plaintext
	bool badtext = checkBadChars(textBuffer, alphabet);
	bool badkey = checkBadChars(keyBuffer, alphabet);		 
	if(badtext == true || badkey == true){
		error("ENC_CLIENT: Bad character found", 1);
	}

	// replace the \n after plaintext for future extraction in server
	textBuffer[strcspn(textBuffer, "\n")] = '@';

	// put all data together and send it at once later
        memset(largeBuffer, '\0', sizeof(largeBuffer));
        strcpy(largeBuffer, textBuffer);
        strcat(largeBuffer, keyBuffer);
	
  	// Create a socket
  	socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  	if (socketFD < 0){
    		error("ENC_CLIENT: ERROR opening socket", 1);
  	}

   	// Set up the server address struct
  	setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");


  	// Connect to server
  	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    		error("ENC_CLIENT: ERROR connecting", 2);
  	}



 	// Send message to server
  	// Write to the server
  	char messg[] =  "Connecting from encrypt client\n";
  	int charsWrite = 0;
	charsWrite =  send(socketFD, messg, strlen(messg),0);
	if(charsWrite < 0){
		error("ENC_CLIENT: ERROR writing greeting message to socket", 0);
	}


	// receive verify message from server
	memset(buffer, '\0', sizeof(buffer));
	greetingRead = recv(socketFD, buffer, 31, 0);
	if(greetingRead < 0){
		error("ENC_CLIENT: ERROR reading verify message from enc_server", 1);
	}
	//printf("ENC_CLIENT: Greeting from server: %s", buffer);

	if(strstr(buffer, "decrypt server") != NULL){
		error("ENC_CLIENT: ERROR connecting to the wrong server",2);
	}

	// send all the data
	textWritten = send(socketFD, largeBuffer, strlen(largeBuffer), 0);
        if (textWritten < 0){
               	error("ENC_CLIENT: ERROR writing data to socket", 1);
        }
        if (textWritten < strlen(largeBuffer)){
		//printf("text send: %i\n", textWritten);
        	error("ENC_CLIENT: WARNING: Not all data written to socket!\n", 1);
        }
	

  	// Get cipher message from server
  	// Clear out the buffer again for reuse
  	memset(cipherBuffer, '\0', sizeof(cipherBuffer));
	memset(smallBuffer, '\0', sizeof(smallBuffer));
	
	while(strstr(smallBuffer, "\n") == NULL){
		memset(smallBuffer, '\0', sizeof(smallBuffer));

  		// Read data from the socket
  		cipherRead = recv(socketFD, smallBuffer, sizeof(smallBuffer), 0); 
  		if (cipherRead < 0){
   			error("ENC_CLIENT: ERROR reading from socket", 0);
  		}
		/*
		if(cipherRead < strlen(smallBuffer)){
			if(strlen(cipherBuffer) != sizeof(cipherBuffer)){
				//error("ENC_CLIENT: WARNING: Not all Cipher text written to socket!\n", 0);
			}
		}
		*/
		strcat(cipherBuffer, smallBuffer);
	}
	//printf("cipherRead: %i\n", cipherRead);
  	//printf("ENC_CLIENT: I received this from the server: \"%s\"\n",cipherBuffer);
	printf("%s", cipherBuffer);

	// close files
	close(plainFile);
	close(keyFile);
  	// Close the socket
 	close(socketFD); 
  	return 0;

}
