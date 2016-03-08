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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bolos.h"

const unsigned char SETUP_1[] = { 0xD0,0x20,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x18,0xF4,0x3F,0x95,0xA2,0x17,0xEF,0xED,0xE0,0xA8,0xD9,0x8D,0xAC,0x35,0x7E,0x3B,0x25,0x01,0xE7,0x9C,0x39,0x58,0xB9,0xD7,0xE1,0x52,0x38,0xD4,0x3A,0x68,0x07,0xC3,0x97,0x68,0x0E,0xB8,0x05,0xBC,0x0E,0x95,0xE2,0xB6,0x5D,0x9E,0x49,0xB1,0xB0,0x45 };

const unsigned char SETUP_2[] = { 0xD0,0x22,0x00,0x00,0x2B,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x20,0xB2,0x50,0x06,0xC5,0x89,0xF0,0xDC,0xF1,0xBB,0xB7,0x5B,0xAA,0x15,0x42,0xA5,0xE6,0xCF,0x30,0x09,0x95,0xF0,0x04,0x6D,0xE5,0x9C,0xC6,0x41,0xC0,0x79,0x8D,0x9D,0x48,0x90,0x06};

const unsigned char SETUP_3[] = { 0xe0,0x20,0x00,0x00,0x2C,0x01,0x0A,0x6F,0xC4,0x04,0x31,0x32,0x33,0x34,0x00,0x20,0xd2,0x5e,0x3c,0x17,0xd4,0x7a,0x04,0x31,0xd2,0x1c,0x45,0x4b,0xb8,0x8e,0x98,0x51,0x83,0xef,0xfc,0x5e,0x4a,0x54,0x5f,0xF2,0xbf,0xf9,0x8d,0x51,0xad,0xc6,0x01,0xb7,0x00 };

const unsigned char VERIFY_PIN[] = { 0xe0,0x22,0x00,0x00,0x04,0x31,0x32,0x33,0x34 };

const unsigned char GET_PUBLIC_KEY[] = { 0xe0, 0x40, 0x00, 0x00, 0x0d, 0x03, 0x80, 0x00, 0x00, 0x44, 0x80, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78 };

int send_apdu(char* apdu, const char *data, size_t size) {
	int i;
	int result;
	for (int i=0; i<size; i++) {
		apdu[i] = data[i];
	}
	result = bls_wallet_call(apdu);	
	return (result && (apdu[result - 2] == 0x90) && (apdu[result - 1] == 0x00));
} 

int check_wallet_state() {
	bls_debug("Check wallet state");
	int state = bls_wallet_get_state();
	if ((state & BLS_WALLET_STATE_INITIALIZED) != 0) {
		bls_debug("wallet initialized");
	}
	if ((state & BLS_WALLET_STATE_LOCKED) != 0) {
		bls_debug("wallet locked");
	}
	return state;
}

int main(int argc, char **argv) {
	uint8_t apdu[260];
	uint32_t path[3];
	uint8_t chainCode[32];
	uint8_t address[100];
	bls_ecfp_public_key_t publicKey;
	bls_ecfp_private_key_t privateKey;
	check_wallet_state();
	if (!send_apdu(apdu, SETUP_1, sizeof(SETUP_1))) {
		bls_debug("Fail setup1");
		return 0;
	}
	bls_debug("setup 1 ok");
	if (!send_apdu(apdu, SETUP_2, sizeof(SETUP_2))) {
		bls_debug("Fail setup2");
		return 0;
	}
	bls_debug("setup 2 ok");
	if (!send_apdu(apdu, SETUP_3, sizeof(SETUP_3))) {
		bls_debug("Fail setup3");
		return 0;
	}	
	bls_debug("restore ok");
	check_wallet_state();
	if (!send_apdu(apdu, VERIFY_PIN, sizeof(VERIFY_PIN))) {
		bls_debug("Fail verify PIN");
		return 0;
	}
	check_wallet_state();
	path[0] = 0x80000044;
	path[1] = 0x80000000;
	path[2] = 0x12345678;
	bls_wallet_derive(BLS_WALLET_DERIVE_PUBLIC | BLS_WALLET_DERIVE_PRIVATE, path, 3, chainCode, &privateKey, &publicKey);	
	bls_debug("wallet derived");
	bls_wallet_get_address(&publicKey, address, sizeof(address), 1);
	bls_debug(address);
	if (!send_apdu(apdu, GET_PUBLIC_KEY, sizeof(GET_PUBLIC_KEY))) {
		bls_debug("Fail get public key");
		return 0;
	}
	int offset = 1 + apdu[0];
	int addressLength = apdu[offset++];
	apdu[offset + addressLength] = '\0';
	bls_debug(apdu + offset);
}

