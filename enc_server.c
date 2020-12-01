// enc_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX 70000

void extractData(char*, char* ,char*);
void encryption(char*, char*, char*);

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
   		 error("ERROR opening socket in enc_server", 1);
  	}

  	// Set up the address struct for the server socket
  	setupAddressStruct(&serverAddress, atoi(argv[1]));

  	// Associate the socket to the port
  	if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    		error("ERROR on binding in enc_server",1);
  	}

  	// Start listening for connetions. Allow up to 5 connections to queue up
  	listen(listenSocket, 5); 
  
  	// Accept a connection, blocking if one is not available until one connects
  	while(1){
    		// Accept the connection request which creates a connection socket
    		connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    		if (connectionSocket < 0){
      			error("ERROR on accept in enc_server",1);
    		}

    		//printf("ENC_SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr),ntohs(clientAddress.sin_port));

		// fork
		spawnpid = fork();
		switch(spawnpid){
		   case -1:
			error("ENC_SERVER: fork() failed",1);
			exit(1);
		   case 0:		
    				
    			memset(buffer2, '\0', sizeof(buffer2));
		
			// read greeting massage to verify the the corrent client	
			int greetingRead = 0, read = 0;
			greetingRead = recv(connectionSocket, buffer2, 31, 0);			
			if(greetingRead < 0){
				error("ENC_SERVER: ERROR reading greeting from socket in enc_server\n",1);
				exit(1);
			}
			//printf("greetingRead: %i\n", greetingRead);	
			//printf("Enc_server: greeting from client: %s", buffer2);
			/*if(strstr(buffer2, "decrypt client") != NULL){
				error("ENC_SERVER: ERROR connecting to the wrong client",2);
				exit(2);
			}*/

			// send verify message to client
			char messg[] = "Connecting from encrypt server\n";
			greetingWritten = send(connectionSocket, messg, strlen(messg), 0);
			if(greetingWritten < 0){
				error("ENC_SERVER: ERROR writting to socket\n", 1);
			}
			
			//~~
			/*
			if(strstr(buffer2, "decrypt client") != NULL){
                                error("ENC_SERVER: ERROR connecting to the wrong client",2);
                                exit(2);
                        }

	*/

			memset(smallBuffer, '\0', sizeof(smallBuffer));
			memset(textBuffer, '\0',sizeof(textBuffer));

    			// Read the plaintext and key from the socket
    			while(strstr(smallBuffer, "\n") == NULL){

				// read every 1000 characters
				memset(smallBuffer, '\0', sizeof(smallBuffer));
    				charsRead = recv(connectionSocket, smallBuffer,sizeof(smallBuffer), 0); 
    				if (charsRead < 0){
      					error("ERROR reading from socket in enc_server",1);
    				}
				if(charsRead < strlen(smallBuffer)){
					if(strlen(largeBuffer) != sizeof(largeBuffer)){
						error("Not all data received",1);
					}
			
				}
				strcat(largeBuffer, smallBuffer);
			}
    			//printf("ENC_SERVER: I received this from the client: \"%s\"\n", largeBuffer);

			// extract data
			memset(keyBuffer, '\0' ,sizeof(keyBuffer));
			memset(textBuffer, '\0', sizeof(textBuffer));
			extractData(largeBuffer, keyBuffer, textBuffer);			
			// encrypt data
			memset(cipherBuffer, '\0' , sizeof(cipherBuffer));
			encryption(cipherBuffer, keyBuffer, textBuffer);			
			cipherBuffer[strlen(cipherBuffer)] = '\n';
			
    			// Send cipher text back
    			charsRead = send(connectionSocket, cipherBuffer, strlen(cipherBuffer), 0); 
    			if (charsRead < 0){
      				error("ERROR writing to socket in enc_server",1);
    			}
			if(charsRead < strlen(cipherBuffer)){
				error("Error: Not all data send",1);
			}


			exit(0);
		   default:
    			// Close the connection socket for this client
			childpid = waitpid(-1, &childStatus, 0);
			//printf("The parent is done waiting, The pid that terminated is %d\n", childpid);
			//fflush(stdout);	
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
	//printf("plaintext: [%s]   key: [%s]\n", plaintext, key);	
}

// convert input char array to int array
void charToInt(char* buffer, int * array, char* alphabet){
	int i, j;
	
	// find the location for each char in the buffer by comparing it to the alphabet array
	for(i = 0; i < strlen(buffer); i++){
		for(j = 0; j < strlen(alphabet); j++){
			
			// store the index to a location array if both characters equal, e.g. A is 0, B is 1 etc.
                        if(buffer[i] == alphabet[j]){
				//printf("%i) %c %i\n", i, alphabet[j], j);
                                array[i] = j;
                        }
                }
	}
}

// encryption function
void encryption(char * cipherBuffer, char * key, char * plaintext){
	char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int i , n = 27, n2, location[strlen(plaintext)], keyLocation[strlen(key)];
	char c;
	
	// convert key and plaintext into integer(location) array
	charToInt(plaintext, location, alphabet);		
	charToInt(key, keyLocation, alphabet);
	
	// calculations: add up key and plaintext location, then mod if result > 26(index)
	for(i = 0; i < strlen(plaintext); i++){
		n2 = location[i] + keyLocation[i];
		//if(n2 > n){
			n2  =  n2 % n;
		//}
		cipherBuffer[i] = alphabet[n2];
		//printf("%i) alpha:%c cipher:%c  n2:%i\n", i, alphabet[n2], cipherBuffer[i], n2);
	}
	//printf("cipherBuffer: %s\n", cipherBuffer);
}	

