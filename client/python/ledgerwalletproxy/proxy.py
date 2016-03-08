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

from ledgerwalletproxy.LedgerWalletProxy_pb2 import LedgerWalletProxyRequest, OpenSession, CloseSession, ExchangeWallet, CodeRangeType, ResumeCode, GetNVRAM, GetAttestation
from elftools.elf.elffile import ELFFile
from elftools.elf.constants import P_FLAGS
import os

class ProxyException(Exception):

	def __init__(self, message, reason):
		self.message = message
		self.reason = reason

	def __str__(self):
		return self.message

	def getReason(self):
		return self.reason

class Proxy(object):

	def __init__(self, transport):
		self.transport = transport

	def openSession(self, nvram=None, attestation=None):
		message = LedgerWalletProxyRequest()
		openSession = OpenSession()
		if nvram is not None:
			openSession.nvram = nvram
		if attestation is not None:
			openSession.attestation = attestation
		message.openSession.CopyFrom(openSession)
		response = self.transport.exchange(message)
		if not response.HasField('genericAck'):
			raise ProxyException("Unexpected response", response)

	def closeSession(self):
		message = LedgerWalletProxyRequest()
		message.closeSession.CopyFrom(closeSession)
		response = self.transport.exchange(message)
		if not response.HasField('genericAck'):
			raise ProxyException("Unexpected response", response)

	def getNVRAM(self):
		message = LedgerWalletProxyRequest()
		message.getNVRAM.CopyFrom(GetNVRAM())
		response = self.transport.exchange(message)
		if not response.HasField('getNVRAMAck'):
			raise ProxyException("Unexpected response", response)			
		return response.getNVRAMAck.nvram

	def getAttestation(self):
		message = LedgerWalletProxyRequest()
		message.getNVRAM.CopyFrom(GetAttestation())
		response = self.transport.exchange(message)
		if not response.HasField('getAttestationAck'):
			raise ProxyException("Unexpected response", response)			
		return response.getAttestationAck.attestation

	def runCode(self, fileName, parameters=None, stackSize=2048):
		message = LedgerWalletProxyRequest()
		f = open(fileName, 'rb')
		elffile = ELFFile(f)
		for section in elffile.iter_sections():
			if section.name == '.ledger':		
				message.startCode.signature = section.data()[0:ord(section.data()[1]) + 2]
				break
		if len(message.startCode.signature) == 0:
			raise Exception("Missing code signature")
		message.startCode.stackSize = stackSize
		message.startCode.entryPoint = elffile.header['e_entry']
		message.startCode.parameters = parameters
		for segment in elffile.iter_segments():
			if segment['p_type'] == 'PT_LOAD':
				codeRange = message.startCode.code.add()
				flags = 0
				if ((segment['p_flags'] & P_FLAGS.PF_W) == 0):
					flags = flags | 0x01
				codeRange.flags = flags
				codeRange.start = segment['p_vaddr']
				codeRange.end = segment['p_vaddr'] + segment['p_memsz']
				codeRange.dataLength = segment['p_filesz']
				codeRange.data = segment.data()

		response = self.transport.exchange(message)		
		while response.HasField('logAck'):
			print response.logAck.message
			message = LedgerWalletProxyRequest()
			message.resumeCode.CopyFrom(ResumeCode())
			response = self.transport.exchange(message)
		if response.HasField('startCodeResponseAck'):
			return response.startCodeResponseAck.response
		else:
			raise ProxyException("Unexpected response", response)


	def exchange(self, apdu):
		message = LedgerWalletProxyRequest()
		message.exchangeWallet.apdu = apdu
		response = self.transport.exchange(message)
		if response.HasField('exchangeWalletAck'):
			return response.exchangeWalletAck.response
		else:
			raise ProxyException("Unexpected response", response)

  