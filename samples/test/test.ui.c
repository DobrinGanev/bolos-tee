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
	char entry[100];
	bls_debug("start");
	bls_ui_display_message("test message\non\nseveral lines\nat once");
	bls_ui_display_warning("warning message\non\nseveral lines");
	bls_ui_display_error("error message\non\nseveral lines");
	bls_ui_display_qr("Test QR code\nPay to\n", "15HCzh8AoKRnTWMtmgAsT9TKUPrQ6oh9HQ", 34);
	int result = bls_ui_display_choice("test choice\nalso\non several lines");
	if (result) {
		bls_debug("choice ok");
	}
	result = bls_ui_get_user_entry("test user entry\non several lines", entry, sizeof(entry));
	if (result) {
		bls_debug("entry ok");
		bls_debug(entry);
	}
	bls_debug("end");
}

