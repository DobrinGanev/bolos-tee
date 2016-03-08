/*
*******************************************************************************
*   BOLOS TEE Tools
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*   limitations under the License.
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "secp256k1.h"

int main(int argc, char **argv) {
	secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
	secp256k1_pubkey pubkey;
	unsigned char tmp[32];
	unsigned char out[65];
	size_t length = sizeof(out);
	int i;
	FILE *f;
	f = fopen("/dev/random", "r");
	if (f == NULL) {
		fprintf(stderr, "Error opening /dev/random\n");
		return 1;
	}
	for(;;) {
		if (fread(tmp, sizeof(tmp), 1, f) != 1) {
			fprintf(stderr, "Error reading /dev/random\n");
			return 1;
		}
		if (secp256k1_ec_seckey_verify(ctx, tmp) == 1) {
			break;
		}
       		continue;
	}
	if (secp256k1_ec_pubkey_create(ctx, &pubkey, tmp) != 1) {
		fprintf(stderr, "Error getting public key\n");
		return 1;
	}
	secp256k1_ec_pubkey_serialize(ctx, out, &length, &pubkey, SECP256K1_EC_UNCOMPRESSED); 
	secp256k1_context_destroy(ctx);
	printf("Private key : ");
	for (i=0; i<sizeof(tmp); i++) {
		printf("%.2x", tmp[i]);
	}	
	printf("\n");
	printf("Public key : ");
	for (i=0; i<length; i++) {
		printf("%.2x", out[i]);
	}
	printf("\n");
	return 0;
}

