CS 344 Assignment 5 
YanYan Lee
leeya@oregonstate.edu

To compile: 
	compileall

For the testing script:
	It won't produce the correct ciphertext file, but when I tested separately with the command, it does the concurrency, and same as the dec_client: 

	./enc_client plaintext2 key70000 56840 > ciphertext2; ./enc_client plaintext1 key70000 56840 > ciphertext1; ./enc_client plaintext3 key70000 56840 > ciphertext3; ./enc_client plaintext4 key70000 56840 > ciphertext4; ./enc_client plaintext5 key70000 56840 > ciphertext5;

	
