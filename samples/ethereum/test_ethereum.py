#******************************************************************************
#*   BOLOS TEE Samples
#*   (c) 2016 Ledger
#*
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*   Unless required by applicable law or agreed to in writing, software
#*   distributed under the License is distributed on an "AS IS" BASIS,
#*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*   See the License for the specific language governing permissions and
#*   limitations under the License.
#********************************************************************************

# Run with proxy_IP proxy_port signed_app_eth_confirm

from ledgerwalletproxy.transport import Transport
from ledgerwalletproxy.proxy import Proxy
import os
import sys

INIT_1 = "D020000038000000000000000118F43F95A217EFEDE0A8D98DAC357E3B2501E79C3958B9D7E15238D43A6807C397680EB805BC0E95E2B65D9E49B1B045".decode('hex')
INIT_2 = "D02200002B000000020000000120B25006C589F0DCF1BBB75BAA1542A5E6CF300995F0046DE59CC641C0798D9D489006".decode('hex')
SETUP = "E0200000100B020005083030303030303030000000".decode('hex')
VERIFY_PIN = "E02200000401020304".decode('hex')

TEST_ETH_TX = "e8808504e3b2920082520894aea4f5d9319e33436dbdbe015d52982f76d62348878e1bc9bf04000080".decode('hex')

transport = Transport(sys.argv[1], int(sys.argv[2]))

transport.open()
proxy = Proxy(transport)

def initWallet():
	proxy.exchange(INIT_1)
	proxy.exchange(INIT_2)
	proxy.exchange(SETUP)
	nvram = proxy.getNVRAM()
	f = open('nvram.bin', 'wb')
	f.write(nvram)
	f.close()

nvram = None
if os.path.exists('nvram.bin'):
	f = open('nvram.bin', 'rb')
	nvram = f.read()
	f.close()
proxy.openSession(nvram=nvram)

if nvram == None:
	initWallet()

# Unlock
proxy.exchange(VERIFY_PIN)

parameters = "00000000".decode('hex') + chr(len(TEST_ETH_TX) >> 8) + chr(len(TEST_ETH_TX) & 0xff) + TEST_ETH_TX

response = proxy.runCode(fileName=sys.argv[3], parameters=parameters)
print response

