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
 * @brief Simple confirmation logic for an Ethereum Hardware Wallet
 * @file app_eth_confirm.c
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

#define CURRENTFIELD_CONTENT 0
#define CURRENTFIELD_NONCE 1
#define CURRENTFIELD_GASPRICE 2
#define CURRENTFIELD_STARTGAS 3
#define CURRENTFIELD_TO 4
#define CURRENTFIELD_VALUE 5
#define CURRENTFIELD_DATA 6

const char *MSG[] = {
	"Confirm the following ETH transaction data\n",
	"",
	"GAS PRICE: ",
	"START GAS: ",
	"TO :",
	"VALUE: "
};

#define SCRATCH_SIZE 21

/**
 * @brief Convert an 8 bytes amount to a BCD string, using the Double Dabble algorithm
 * @param [in] amount 8 bytes amount
 * @param [out] target string to display
 * @return size of the converted amount
 */
uint32_t display_amount(uint8_t *amount, uint8_t *target) {
  uint16_t scratch[SCRATCH_SIZE];
  uint32_t offset = 0, nonZero = 0, i, targetOffset = 0, workOffset, j, nscratch = SCRATCH_SIZE, smin = nscratch - 2;

  for (i=0; i<SCRATCH_SIZE; i++) {
    scratch[i] = 0;
  }
  for (i=0; i<8; i++) {
    for (j=0; j<8; j++) {
      uint32_t k;
      uint16_t shifted_in = (((amount[i] & 0xff) & ((1 << (7 - j)))) != 0) ? 1 : 0;
      for (k=smin; k<nscratch; k++) {
        scratch[k] += ((scratch[k] >= 5) ? 3 : 0);
      }
      if (scratch[smin] >= 8) {
        smin -= 1;
      }
      for (k=smin; k < nscratch - 1; k++) {
        scratch[k] = ((scratch[k] << 1)&0xF)|((scratch[k + 1] >= 8) ? 1 : 0);
      }
      scratch[nscratch - 1] = ((scratch[nscratch - 1]<<1)&0x0F)|(shifted_in == 1 ? 1 : 0);
    }
  }

  for (i=0; i<nscratch - 1; ++i) {
  	if (scratch[i] != 0) {
  		break;
  	}  	
  }
  nscratch -= i;
  for (j=0; j<nscratch; j++) {
  	target[targetOffset++] = scratch[i + j] + '0';
  }

  target[targetOffset] = '\0';
  return targetOffset - 1;
}

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

int main(int argc, char **argv) {
	bls_ecfp_private_key_t privateKey;
	uint8_t result[200];
	uint8_t keyData[65];
	uint32_t path[5];
	uint8_t tmp[4];
	uint8_t hash[32];
	uint8_t debug[200];
	uint8_t tx[200];
	uint32_t i;
	uint32_t txLength;

	/* Note : bls_get_input_parameters_length fails on the current release, so we are not using it */
	bls_copy_input_parameters(tmp, 0, 4); // derivation index
	bls_copy_input_parameters(debug, 4, 2); // txLength
	txLength = (debug[0] << 8) + debug[1];		
	bls_copy_input_parameters(tx, 6, txLength); // raw tx to validate

	bool firstField = true;
	uint32_t offset = 0;
	uint32_t currentField = 0;
	uint32_t messageOffset = 0;
	uint8_t message[300];
	/* Parse the RLP encoded raw transaction and fetch the interesting elements to display */
	while (offset != txLength) {
		uint32_t fieldLength;
		uint32_t deltaOffset;
		bool list;
		if (!rlpDecodeLength(tx + offset, txLength - offset, &fieldLength, &deltaOffset, &list)) {
			bls_debug("Invalid RLP structure, aborting");
			result[0] = 0;
			bls_set_return(result, 1);
			return 0;		
		}
		offset += deltaOffset;
		if (offset > txLength) {
			bls_debug("Invalid TX, aborting");
			result[0] = 0;
			bls_set_return(result, 1);
			return 0;					
		}		
		/* Skip the nonce and data */
		/* Data validation can be added for specific transactions and contracts */
		if ((currentField != CURRENTFIELD_NONCE) && (currentField != CURRENTFIELD_DATA)) {
			if (MSG[currentField][0] != 0) {
				localcopy(message + messageOffset, MSG[currentField], locallength(MSG[currentField]));
				messageOffset += locallength(MSG[currentField]);
			}

			/* Display the destination address as a binary buffer and the remaining parts as numbers */
			if ((fieldLength != 0) && (currentField != 0)) {
				if (currentField != CURRENTFIELD_TO) {
					uint8_t amount[8];
					if (fieldLength > 8) {
						bls_debug("Amount too big, aborting");
						result[0] = 0;
						bls_set_return(result, 1);
						return 0;				
					}
					for (uint32_t i=0; i<8; i++) {
						amount[i] = 0;
					}
					localcopy(amount + (8 - fieldLength), tx + offset, fieldLength);
					messageOffset += display_amount(amount, message + messageOffset);
				}
				else {
					uint32_t blockOffset = 0;
					message[messageOffset++] = '\n';
					if (fieldLength > 20) {
						bls_debug("Destination too big, aborting");
						result[0] = 0;
						bls_set_return(result, 1);
						return 0;				
					}
					/* The current BOLOS TEE has little memory available for the screen buffer */
					/* We avoid problems by formatting strings that are not too long */					
					while (blockOffset != fieldLength) {
						uint32_t chunkSize = (blockOffset + 8 > fieldLength ? fieldLength - blockOffset : 8);
						dump(message + messageOffset, tx + offset + blockOffset, chunkSize);
						messageOffset += chunkSize;
						blockOffset += chunkSize;
						if (blockOffset != fieldLength) {
							message[messageOffset++] = '\n';
						}
					}
				}
			}		
			message[messageOffset++] = '\n';
		}

		currentField++;
		if (!firstField) {
			offset += fieldLength;
		}
		else {
			firstField = false;
		}
		if (offset > txLength) {
			bls_debug("Invalid TX, aborting");
			result[0] = 0;
			bls_set_return(result, 1);
			return 0;					
		}
	}

	/* Helloworld debugging */

	//message[messageOffset] = '\0';
	//bls_debug(message);

	/* Prompt the user, abort if not ok */

	int choiceResult = bls_ui_display_choice(message);
	if (!choiceResult) {
		bls_debug("Aborted by user");
		result[0] = 0;
		bls_set_return(result, 1);
		return 0;				
	}

	/* If ok, derive the wallet key on SLIP 44 path */

	path[0] = 0x8000002C; // 44'
	path[1] = 0x8000003C; // 60'
	path[2] = 0x80000000; // 0'
	path[3] = 0x00000000; // 0
	path[4] = (tmp[0] << 24) | (tmp[1] << 16) | (tmp[2] << 8) | tmp[3]; // user index
	/* Derivation can fail if the wallet is not created, or locked */
	/* If it's locked, enter the session PIN previously with the wallet logic E0220000XXPPPPPPPP */
	if (!bls_wallet_derive(BLS_WALLET_DERIVE_PRIVATE, path, 5, NULL, &privateKey, NULL)) {
		bls_debug("Error deriving key, aborting");
		result[0] = 0;
		bls_set_return(result, 1);
		return 0;
	}

	/* Compute the Keccak-256 hash of the transaction */

	SHA3_256(hash, tx, txLength);

	/* And sign it using secp256k1 - passing BLS_SHA256 and BLS_RND_RFC6979 makes the legacy BOLOS wrapper happy */

	if (!bls_ecdsa_sign(&privateKey, BLS_SIGN | BLS_LAST | BLS_RND_RFC6979, BLS_SHA256, hash, sizeof(hash), result + 1)) {
		bls_debug("Error signing, aborting");
		result[0] = 0;
		bls_set_return(result, 1);
		return 0;		
	}

	/* Finally send back the signature */

	result[0] = 0x01;

	bls_set_return(result, 1 + result[2] + 2);	
	return 0;
}

