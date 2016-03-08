#******************************************************************************
#*   BOLOS TEE Client
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

import socket
import struct
from LedgerWalletProxy_pb2 import LedgerWalletProxyResponse

class Transport(object):
	def __init__(self, ip, port):
		self.dest = (ip, port)

	def open(self):
		self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.socket.connect(self.dest)

	def exchange(self, message):
		serialized = message.SerializeToString()
		self.socket.send(struct.pack(">H", len(serialized)))
		self.socket.send(serialized)
		size = ""
		while len(size) <> 2:
			size = size + self.socket.recv(2 - len(size))
		size = struct.unpack(">H", size)[0]
		data = ""
		while len(data) <> size:
			data = data + self.socket.recv(size - len(data))
		result = LedgerWalletProxyResponse()
		result.ParseFromString(data) 	
		return result
		
	
	def close(self):
		self.socket.close()	

