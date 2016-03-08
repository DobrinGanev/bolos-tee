/*
*******************************************************************************    
*   BOLOS TEE Samples
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

/** 
 * @brief Simple address generation logic for an Ethereum Hardware Wallet
 * @file app_eth_address.c
 * @author Ledger Firmware Team <hello@ledger.fr>
 * @version 1.0
 * @date 8th of March 2016
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bolos.h"
#include "sha3.h"
#include "ethUtils.h"

/**
 * @brief memcpy/memmove when you need one, and libc fails
 * @param [out] dest destination buffer
 * @param [in] src source buffer
 * @param [length] length of the data to copy
 */
void localcopy(uint8_t *dest, uint8_t *src, uint32_t length) {
	for (uint32_t i=0; i<length; i++) {
		dest[i] = src[i];
	}
}

/**
 * @brief strlen when you need one, and libc fails
 * @param [in] string string to compute the length for
 * @return size of the string
 */
uint32_t locallength(uint8_t *string) {	
	uint32_t i = 0;
	while (string[i++] != 0);
	return i - 1;
}

const unsigned char *HEX = "0123456789ABCDEF";

/**
 * @brief convert a binary buffer to a hex string
 * @param [out] dest destination buffer
 * @param [in] src source buffer
 * @param [length] length of the data to copy
 */
void dump(uint8_t *dest, uint8_t *src, uint32_t length) {
	for (uint32_t i=0; i<length; i++) {
		dest[2 * i] = HEX[(src[i] >> 4) & 0x0f];
		dest[2 * i + 1] = HEX[src[i] & 0x0f];
	}
	dest[2 * length] = '\0';
}

#define MSG "Your ethereum address"

int main(int argc, char **argv) {
	bls_ecfp_public_key_t publicKey;
	uint8_t result[200];
	uint8_t keyData[65];
	uint32_t path[5];
	uint8_t tmp[4];
	uint8_t hash[32];
	uint8_t encodedAddress[50];
	uint8_t encodedAddressLength;
	uint8_t iban[60];
	uint8_t debug[200];
	uint32_t checksum;
	uint32_t i;
	uint32_t messageOffset = 0;
	uint8_t message[300];	

	bls_copy_input_parameters(tmp, 0, 4); // derivation index
	path[0] = 0x8000002C; // 44'
	path[1] = 0x8000003C; // 60'
	path[2] = 0x80000000; // 0'
	path[3] = 0x00000000; // 0
	path[4] = (tmp[0] << 24) | (tmp[1] << 16) | (tmp[2] << 8) | tmp[3];
	/* Derivation can fail if the wallet is not created, or locked */
	/* If it's locked, enter the session PIN previously with the wallet logic E0220000XXPPPPPPPP */	
	if (!bls_wallet_derive(BLS_WALLET_DERIVE_PUBLIC, path, 5, NULL, NULL, &publicKey)) {
		bls_debug("Error deriving key, aborting");
		result[0] = 0;
		bls_set_return(result, 1);
		return 0;
	}
	/* Retrieve the uncompressed public key */
	bls_ecfp_get_public_component(&publicKey, keyData);
	/* Hash it using Keccak-256 */
	SHA3_256(hash, keyData + 1, 64);
	/* Compute the associated IBAN */
	encodedAddressLength = encode_base36(hash + 32 - 20, 20, encodedAddress + 4, sizeof(encodedAddress) - 4);
	encodedAddress[encodedAddressLength + 4] = 0;
	/* Add the IBAN checksum */
	checksum = getIBANChecksum("XE", encodedAddress + 4, encodedAddressLength);	
	encodedAddress[0] = 'X';
	encodedAddress[1] = 'E';
	encodedAddress[2] = '0' + (checksum / 10);
	encodedAddress[3] = '0' + (checksum % 10);
	/* And format the URI to be encoded into the QR code */
	localcopy(iban, "iban:", 5);
	localcopy(iban + 5, encodedAddress, encodedAddressLength + 4);
	/* Display the address and QR code of the IBAN to the user */
	localcopy(message + messageOffset, MSG, locallength(MSG));
	messageOffset += locallength(MSG);
	message[messageOffset++] = '\n';
	message[messageOffset++] = '0';
	message[messageOffset++] = 'x';
	/* The current BOLOS TEE has little memory available for the screen buffer */
	/* We avoid problems by formatting strings that are not too long */					
	dump(message + messageOffset, hash + 32 - 20, 8);
	messageOffset += 8;
	message[messageOffset++] = '\n';
	dump(message + messageOffset, hash + 32 - 20 + 8, 8);
	messageOffset += 8;
	message[messageOffset++] = '\n';
	dump(message + messageOffset, hash + 32 - 20 + 16, 4);
	messageOffset += 4;
	message[messageOffset++] = '\0';
	/* A future version of the API will let the user specify the size of the QR code */
 	bls_ui_display_qr(message, iban, encodedAddressLength + 4 + 5);
	/* Finally send back the address and the IBAN */
	result[0] = 1;
	localcopy(result + 1, hash + 32 - 20, 20);
	localcopy(result + 1 + 20, encodedAddress, encodedAddressLength + 4);
	bls_set_return(result, 1 + 20 + encodedAddressLength + 4);	
}

