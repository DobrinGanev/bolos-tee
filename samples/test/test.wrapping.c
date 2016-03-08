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

int main(int argc, char **argv) {
	unsigned char data[256];
	unsigned char data2[256];
	//int scope = BLS_SCOPE_DEVICE;
	//int scope = BLS_SCOPE_SESSION;
	int scope = BLS_SCOPE_APPLICATION;
	bls_copy_input_parameters(data, 0, 1);
	if (data[0] != 0x00) {
		unsigned char size = data[0];
		bls_copy_input_parameters(data, 1, size);
		int result = bls_unwrap(scope, data, size, data2, sizeof(data2));
		if (result == 0) {
			bls_debug("Unwrap fail wat");
		}
		else { 
			bls_set_return(data2, result);
		}
	}
	else {
		int i;
		for (i=0; i<10; i++) {
			data[i] = i;
		}
		int result = bls_wrap(scope, data, 10, data2, sizeof(data2));
		if (result == 0) {
			bls_debug("Wrap fail");
		}
		else {
			bls_set_return(data2, result);
		}
	}
}

