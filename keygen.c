// key gen.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char** argv){
	int i, nChars, n;
	char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	char * key;

	// if not enough arguments 
	if(argc < 2){
		fprintf(stderr, "integer argument needed for generating a key\n");
		exit(1);
	}
	
	srand(time(NULL));

	nChars = atoi(argv[1]);
	key = (char*)malloc(sizeof(char)* nChars);
	//memset(key, '\0', sizeof(key)*nChars);
	for(i = 0; i < nChars; i++){
		n = rand() % 27;
		key[i] = alphabet[n];
	}		
	printf("%s\n", key);
	
	return 0;
} 
