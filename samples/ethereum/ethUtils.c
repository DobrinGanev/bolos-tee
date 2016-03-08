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
 * @brief Utilities for an Ethereum Hardware Wallet logic
 * @file ethUtils.h
 * @author Ledger Firmware Team <hello@ledger.fr>
 * @version 1.0
 * @date 8th of March 2016
 */

#include "ethUtils.h"

uint32_t decode_base36(uint8_t *in, uint32_t inLength, uint8_t *out, uint32_t outLength) {
	uint8_t buffer[100], tmp[100];
	uint32_t i, j, startAt, zeroCount = 0;
	for (i=0; i<inLength; i++) {
		if ((in[i] >= '0') && (in[i] <= '9')) {
			tmp[i] = (in[i] - '0');
		}
		else
		if ((in[i] >= 'A') && (in[i] <= 'Z')) {
			tmp[i] = (in[i] - 'A' + 10);
		}
		else {
			return 0;
		}
	}
	while ((zeroCount < inLength) && (tmp[zeroCount] == 0)) {
		++zeroCount;
	}
	j = inLength;
	startAt = zeroCount;
	while(startAt < inLength) {
		uint32_t remainder = 0;
		uint32_t divLoop;
		for (divLoop = startAt; divLoop < inLength; divLoop++) {
			uint32_t digit256 = (uint32_t)(tmp[divLoop] & 0xff);
			uint32_t tmpDiv = remainder * 36 + digit256;
			tmp[divLoop] = (uint8_t)(tmpDiv / 256);
			remainder = (tmpDiv % 256);
		}
		if (tmp[startAt] == 0) {
			++startAt;
		}
		buffer[--j] = (unsigned char)remainder;
	}
	while ((j < inLength) && (buffer[j] == 0)) {
		++j;
	}
	inLength = inLength - (j - zeroCount);
	if (outLength < inLength) {
		return 0;
	}
	memcpy(out, buffer + j - zeroCount, inLength);
	return inLength;
}

uint32_t encode_base36(uint8_t *in, uint32_t inLength, uint8_t *out, uint32_t outLength) {
        uint8_t buffer[100], tmp[100];
        uint32_t i, j, startAt, zeroCount = 0;
	if (inLength > sizeof(tmp)) {
		return 0;
	}
	memcpy(tmp, in, inLength);
        while ((zeroCount < inLength) && (tmp[zeroCount] == 0)) {
                ++zeroCount;
        }
        j = 2 * inLength;
        startAt = zeroCount;
        while(startAt < inLength) {
                uint32_t remainder = 0;
                uint32_t divLoop;
                for (divLoop = startAt; divLoop < inLength; divLoop++) {
                        uint32_t digit256 = (uint32_t)(tmp[divLoop] & 0xff);
                        uint32_t tmpDiv = remainder * 256 + digit256;
                        tmp[divLoop] = (uint8_t)(tmpDiv / 36);
                        remainder = (tmpDiv % 36);
                }
                if (tmp[startAt] == 0) {
                        ++startAt;
                }
		if (remainder <= 10) {
			buffer[--j] = ('0' + remainder);
		}
		else {
			buffer[--j] = ('A' + remainder - 10); 
		}
        }
        while ((j < (2 * inLength)) && (buffer[j] == '0')) {
                ++j;
        }
	while (zeroCount -- > 0) {
		buffer[--j] = '0';
	}
        inLength = 2 * inLength - j;
        if (outLength < inLength) {
                return 0;
        }
        memcpy(out, buffer + j, inLength);
        return inLength;
}

bool verifyIBANChecksum(uint8_t *in, uint32_t inLength) {
	uint32_t i, mod = 0;	
	for (i=4; i<inLength + 4; i++) {
		uint8_t x = in[i % inLength];
		if ((x >= '0') && (x <= '9')) {
			mod = (10 * mod + (x - '0')) % 97;
		} 	
		else
		if ((x >= 'A') && (x <= 'Z')) {
			mod = (100 * mod + (x - 'A' + 10)) % 97;
		}
		else {
			return false;
		}
	}
	return (mod == 1);
}

uint32_t getIBANChecksum(uint8_t *prefix, uint8_t *in, uint32_t inLength) {
        uint32_t i, mod = 0;
        for (i=0; i<inLength + 4; i++) {
                uint8_t x;
		if (i == inLength) {
			x = prefix[0];
		}
		else
		if (i == inLength + 1) {
			x = prefix[1];
		}
		else
		if (i > inLength + 1) {
			x = '0';
		} 
		else {
			x = in[i];
		}
                if ((x >= '0') && (x <= '9')) {
                        mod = (10 * mod + (x - '0')) % 97;
                }      
                else
                if ((x >= 'A') && (x <= 'Z')) {
                        mod = (100 * mod + (x - 'A' + 10)) % 97;
                }
                else {
                        return 0;
                }
        }
	return (98 - mod);
}

bool rlpDecodeLength(uint8_t *buffer, uint32_t bufferLength, uint32_t *fieldLength, uint32_t *offset, bool *list) {
	if (*buffer <= 0x7f) {
		*offset = 1;
		*fieldLength = 1;
		*list = false;
	}
	else
	if (*buffer <= 0xb7) {
		*offset = 1;
		*fieldLength = *buffer - 0x80;
		*list = false;
	}
	else
	if (*buffer <= 0xbf) {
		*offset = 1 + (*buffer - 0xb7);
		*list = false;
		switch(*buffer) {
			case 0xb8:
				*fieldLength = *(buffer + 1);
				break;
			case 0xb9:
				*fieldLength = (*(buffer + 1) << 8) + *(buffer + 2);
				break;
			case 0xba:
				*fieldLength = (*(buffer + 1) << 16) + (*(buffer + 2) << 8) + *(buffer + 3);
				break;
			case 0xbb:
				*fieldLength = (*(buffer + 1) << 24) + (*(buffer + 2) << 16) + (*(buffer + 3) << 8) + *(buffer + 4);
				break;
			default:
				return false; // arbitrary 32 bits length limitation
		}
	}
	else
	if (*buffer <= 0xf7) {
		*offset = 1;
		*fieldLength = *buffer - 0xc0;
		*list = true;
	}
	else {
		*offset = 1 + (*buffer - 0xf7);
		*list = true;
		switch(*buffer) {
			case 0xf8:
				*fieldLength = *(buffer + 1);
				break;
			case 0xf9:
				*fieldLength = (*(buffer + 1) << 8) + *(buffer + 2);
				break;
			case 0xfa:
				*fieldLength = (*(buffer + 1) << 16) + (*(buffer + 2) << 8) + *(buffer + 3);
				break;
			case 0xfb:
				*fieldLength = (*(buffer + 1) << 24) + (*(buffer + 2) << 16) + (*(buffer + 3) << 8) + *(buffer + 4);
				break;
			default:
				return false; // arbitrary 32 bits length limitation
		}
	}
	return true;
}
