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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief Decode a Base 36 encoded buffer
 * @param [in] buffer source buffer
 * @param [in] inLength size of the buffer to decode
 * @param [out] out destination buffer
 * @param [in] outLength size of the destination buffer
 * @return size of the decoded content
 */
uint32_t decode_base36(uint8_t *in, uint32_t inLength, uint8_t *out, uint32_t outLength);

/**
 * @brief Encode a buffer using Base 36
 * @param [in] buffer source buffer
 * @param [in] inLength size of the buffer to encode
 * @param [out] out destination buffer
 * @param [in] outLength size of the destination buffer
 * @return size of the encoded content
 */
uint32_t encode_base36(uint8_t *in, uint32_t inLength, uint8_t *out, uint32_t outLength);

/**
 * @brief Verify an IBAN checksum - see https://en.wikipedia.org/wiki/International_Bank_Account_Number
 * @param [in] in IBAN
 * @param [in] inLength size of the IBAN to check
 * @return true if the checksum is correct, or false
 */
bool verifyIBANChecksum(uint8_t *in, uint32_t inLength); 

/**
 * @brief Compute an IBAN checksum - see https://en.wikipedia.org/wiki/International_Bank_Account_Number
 * @param [in] prefix 2 letters IBAN prefix
 * @param [in] in data to include in the IBAN
 * @param [in] inLength size of the data to include in the IBAN
 * @return IBAN checksum
 */
uint32_t getIBANChecksum(uint8_t *prefix, uint8_t *in, uint32_t inLength); 

/**
 * @brief Decode an RLP encoded field - see https://github.com/ethereum/wiki/wiki/RLP
 * @param [in] buffer buffer containing the RLP encoded field to decode
 * @param [in] bufferLength size of the buffer
 * @param [out] fieldLength length of the RLP encoded field
 * @param [out] offset offset to the beginning of the RLP encoded field from the buffer
 * @param [out] list true if the field encodes a list, false if it encodes a string
 * @return true if the RLP header is consistent
 */
 bool rlpDecodeLength(uint8_t *buffer, uint32_t bufferLength, uint32_t *fieldLength, uint32_t *offset, bool *list);
