// dec server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX 140000

void extractData(char*, char* ,char*);
void decryption(char*, char*, char*);

// Error function used for reporting issues
void error(const char *msg, int value) {
  	perror(msg);
  	exit(value);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  	// Clear out the address struct
	memset((char*) address, '\0', sizeof(*address)); 

  	// The address should be network capable
  	address->sin_family = AF_INET;
  	// Store the port number
  	address->sin_port = htons(portNumber);
	//printf("port in server : %i\n", ntohs(address->sin_port));
  	// Allow a client at any address to connect to this server
  	address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]){
  	int connectionSocket, charsRead, greetingWritten;
	char keyBuffer[MAX], textBuffer[MAX], cipherBuffer[MAX], buffer2[50];
	char largeBuffer[MAX*2], smallBuffer[1000];
  	struct sockaddr_in serverAddress, clientAddress;
  	socklen_t sizeOfClientInfo = sizeof(clientAddress);
	pid_t spawnpid, childpid;	
	int childStatus;

  	// Check usage & args
  	if (argc < 2) { 
    		fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    		exit(1);
  	} 
  
  	// Create the socket that will listen for connections
  	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  	if (listenSocket < 0) {
   		 error("DEC_SERVER: ERROR opening socket", 1);
  	}

  	// Set up the address struct for the server socket
  	setupAddressStruct(&serverAddress, atoi(argv[1]));

  	// Associate the socket to the port
  	if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    		error("DEC_SERVER: ERROR on binding",1);
  	}

  	// Start listening for connetions. Allow up to 5 connections to queue up
  	listen(listenSocket, 5); 
  
  	// Accept a connection, blocking if one is not available until one connects
  	while(1){
    		// Accept the connection request which creates a connection socket
    		connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    		if (connectionSocket < 0){
      			error("DEC_SERVER: ERROR on accept",1);
    		}

    		//printf("ENC_SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr),ntohs(clientAddress.sin_port));

		// fork
		spawnpid = fork();
		switch(spawnpid){
		   case -1:
			error("DEC_SERVER: fork() failed",1);
			exit(1);
		   case 0:		
    			
    			memset(buffer2, '\0', sizeof(buffer2));
		
			// read greeting massage to verify the the corrent client	
			int greetingRead = 0, read = 0;
			greetingRead = recv(connectionSocket, buffer2, 31, 0);			
			if(greetingRead < 0){
				error("DEC_SERVER: ERROR reading greeting from socket",1);
			}
			//printf("greetingRead: %i\n", greetingRead);	
			//printf("Enc_server: greeting from client: %s", buffer2);
			/*
			if(strstr(buffer2, "encrypt client") != NULL){
				error("DEC_SERVER: ERROR connecting to the wrong client",1);
			}
			*/
			// send verify message to client
			char messg[] = "Connecting from decrypt server\n";
			greetingWritten = send(connectionSocket, messg, strlen(messg), 0);
			if(greetingWritten < 0){
				error("DEC_SERVER: ERROR writting to socket", 1);
			}
			

			memset(smallBuffer, '\0', sizeof(smallBuffer));
			memset(textBuffer, '\0',sizeof(textBuffer));

    			// Read the plaintext and key from the socket
    			while(strstr(smallBuffer, "\n") == NULL){

				// read every 1000 characters
				memset(smallBuffer, '\0', sizeof(smallBuffer));
    				charsRead = recv(connectionSocket, smallBuffer,sizeof(smallBuffer), 0); 
    				if (charsRead < 0){
      					error("DEC_SERVER: ERROR reading from socket",1);
    				}
				if(charsRead < strlen(smallBuffer)){
					if(strlen(largeBuffer) != sizeof(largeBuffer)){
						error("DEC_SERVER: Not all data received",1);
					}
			
				}
				strcat(largeBuffer, smallBuffer);
			}
    			//printf("ENC_SERVER: I received this from the client: \"%s\"\n", largeBuffer);

			// extract data
			memset(keyBuffer, '\0' ,sizeof(keyBuffer));
			memset(cipherBuffer, '\0', sizeof(textBuffer));
			extractData(largeBuffer, keyBuffer, cipherBuffer);
		
			//printf("after extraction\n");	
			// encrypt data
			memset(textBuffer, '\0' , sizeof(textBuffer));
			decryption(textBuffer, keyBuffer, cipherBuffer);
	
			//printf("after decryption\n");			
			
			textBuffer[strlen(textBuffer)] = '\n';
			
			//printf("textbuffer\n");
    			// Send plain text back
    			charsRead = send(connectionSocket, textBuffer, strlen(textBuffer), 0); 
    			if (charsRead < 0){
      				error("DEC_SERVER: ERROR writing to socket",1);
    			}
			if(charsRead < strlen(textBuffer)){
				error("DEC_SERVER: Error: Not all data send",1);
			}
			//printf("after send\n");

			exit(0);
		   default:
    			// Close the connection socket for this client
			childpid = waitpid(-1, &childStatus, 0);
			//printf("The parent is done waiting, The pid that terminated is %d\n", childpid);
			fflush(stdout);	
    			close(connectionSocket); 
			
    		}
	}
  	// Close the listening socket
  	close(listenSocket); 
  	return 0;
}

// extract data function
void extractData(char* buffer, char* key, char * plaintext){
	char *token;
	char * ptr;
	// extract key and plaintext from buffer
	token = strtok_r(buffer, "@", &ptr);
	strcpy(plaintext, token);
	token = strtok_r(NULL, "\n", &ptr);
	strcpy(key, token);
	//printf("cipherText: [%s]   key: [%s]\n", plaintext, key);	
}

// convert input char array to int array
void charToInt(char* buffer, int * array, char* alphabet){
	int i, j;
	//printf("in char to int\n");	
	//printf("size array: ", sizeof(array));
	// find the location for each char in the buffer by comparing it to the alphabet array
	for(i = 0; i < strlen(buffer); i++){
		for(j = 0; j < strlen(alphabet); j++){	
			// store the index to a location array if both characters equal, e.g. A is 0, B is 1 etc.
			//printf("i: %i j :%i\n", i,j);
			//printf("buffer[%i] = %c\n", i, buffer[i]);
                        if(buffer[i] == alphabet[j]){
				//printf("i: %i j: %i) %c %i\n", i, j, alphabet[j], j);
                                array[i] = j;
                        }
                }
	}
}

// decryption function
void decryption(char * plaintext, char * key, char * ciphertext){
	//printf("in decrytion\n");
	char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int i , n = 27, n2, location[strlen(ciphertext)], keyLocation[strlen(key)];
	char c;

	//printf("cipher: %s length: %i\n", ciphertext, strlen(ciphertext));	
	// convert key and ciphertext into integer(location) array
	charToInt(ciphertext, location, alphabet);		
	charToInt(key, keyLocation, alphabet);

	//printf("after char to int");
	
	// calculations: subtract key from plaintext location, then add 26 if result is negative
	for(i = 0; i < strlen(ciphertext); i++){
		n2 = location[i] - keyLocation[i];
		if(n2 < 0){
			n2  =  n2 + n;
		}
		plaintext[i] = alphabet[n2];
		//printf("%i) alpha:%c cipher:%c  n2:%i\n", i, alphabet[n2], cipherBuffer[i], n2);
	}
	//printf("plainBuffer: %s\n", plaintext);
}	

