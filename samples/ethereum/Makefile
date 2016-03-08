all: 	app_eth_address app_eth_confirm

app_eth_address: app_eth_address.c ethUtils.c sha3_mod.c Keccak-readable-and-compact.c    
	moxiebox-gcc -O2 -o app_eth_address app_eth_address.c ethUtils.c sha3_mod.c Keccak-readable-and-compact.c -I../../api -L../../runtime -lbolos

app_eth_confirm: app_eth_confirm.c ethUtils.c sha3_mod.c Keccak-readable-and-compact.c
	moxiebox-gcc -O2 -o app_eth_confirm app_eth_confirm.c ethUtils.c sha3_mod.c Keccak-readable-and-compact.c -I../../api -L../../runtime -lbolos

clean:
	rm -rf *.o
	rm -rf app_eth_address
	rm -rf app_eth_confirm

