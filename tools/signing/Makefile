all: createKey signMoxie

signMoxie: signMoxie.c sha2.c
	gcc -o signMoxie signMoxie.c sha2.c -lelf -lsecp256k1

createKey: createKey.c
	gcc -o createKey createKey.c -lsecp256k1

clean:
	rm -rf createKey signMoxie 	

