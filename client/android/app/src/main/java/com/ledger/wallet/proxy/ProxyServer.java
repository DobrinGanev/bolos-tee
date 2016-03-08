/*
*******************************************************************************
*   BOLOS TEE 
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


package com.ledger.wallet.proxy;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Vector;

import com.google.protobuf.ByteString;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.BusyAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.CloseSession;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.CodeRangeType;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.ExchangeWallet;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.ExchangeWalletAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.GenericAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.GenericErrorAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.GetAttestation;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.GetAttestationAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.GetNVRAM;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.GetNVRAMAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.LedgerWalletProxyRequest;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.LedgerWalletProxyResponse;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.LogAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.OpenSession;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.ResumeCode;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.StartCode;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.StartCodeErrorAck;
import com.ledger.wallet.proxy.protobuf.LedgerWalletProxyProtobuf.StartCodeResponseAck;
import com.ledger.wallet.proxy.utils.Dump;
import com.ledger.wallet.service.ILedgerWalletService;
import com.ledger.wallet.service.ServiceResult;

import android.os.RemoteException;
import android.util.Log;

public class ProxyServer extends Thread {
	
	private static final int RESPONSE_BUFFER_SIZE = 900000;
	
	private static final byte PROTOCOL_CODERUNTIME = 0x06;
	private static final int MSG_CODERUNTIME_INIT = 0x01;
	private static final int MSG_CODERUNTIME_LOAD_SECTION = 0x02;
	private static final int MSG_CODERUNTIME_RUN_CODE = 0x03;
	private static final int MSG_CODERUNTIME_RESUME_CODE = 0x04;
	private static final int STATUS_CODERUNTIME_OK = 0x01;
	private static final int STATUS_CODERUNTIME_ERROR = 0x80;
	private static final int STATUS_CODERUNTIME_LOG = 0x02;
	
	private ILedgerWalletService ledgerService;
	private InfoLogger logger;
	private ServerSocket serverSocket;
	private int port;
	
	private boolean busy;
	private boolean logPending;
	private byte[] session;
	private byte[] ui;
	private byte[] responseBuffer;
		
	protected ProxyServer(InfoLogger logger, ILedgerWalletService ledgerService, ServerSocket serverSocket, int port) {
		this.logger = logger;
		this.ledgerService = ledgerService;
		this.serverSocket = serverSocket;
		this.port = port;
		responseBuffer = new byte[RESPONSE_BUFFER_SIZE];
		try {
			ServiceResult result = ledgerService.getUI();
			ui = result.getResult();
		}
		catch(RemoteException e) {			
		}		
	}
	
	public static ProxyServer create(InfoLogger logger, ILedgerWalletService ledgerService, int port) {
		try {
			InetSocketAddress address = new InetSocketAddress(port);
			ServerSocket serverSocket = new ServerSocket();
			serverSocket.setReuseAddress(true);
			serverSocket.bind(address);
			return new ProxyServer(logger, ledgerService, serverSocket, port);
		}
		catch(Exception e) {
			Log.d(MainActivity.TAG, "Failed to create server", e);
			return null;
		}
	}
		
	private void closeSession() {
		if (busy && (session != null)) {
			try {
				ledgerService.close(session);
			}
			catch(Exception e1) {					
			}
		}
		busy = false;
		session = null;		
	}
	
	private int readSize(InputStream is) throws Exception {
		int size;
		int x = is.read();
		if (x < 0) {
			throw new RuntimeException("Connection closed");
		}
		size = (x << 8);
		x = is.read();
		if (x < 0) {
			throw new RuntimeException("Connection closed");			
		}
		size |= x;
		return size;
	}
	
	private void writeSize(OutputStream os, int size) throws Exception {
		os.write((size >> 8) & 0xff);
		os.write(size & 0xff);
	}
	
	private byte[] readBuffer(InputStream is, int size) throws Exception {
		byte[] result = new byte[size];
		int offset = 0;
		while (offset != size) {
			int readSize = is.read(result, offset, size - offset);
			if (readSize < 0) {
				throw new RuntimeException("Connection closed");							
			}
			offset += readSize;
		}
		return result;
	}
	
	private void filterFailure(String message, ServiceResult result) {
		if (result.getExceptionMessage() != null) {			
			logger.logInfo("ERROR " + message + ":" + result.getExceptionMessage());
			throw new RuntimeException(message + ":" + result.getExceptionMessage());
		}
	}
	
	private void handleOpenSession(OpenSession openSession, LedgerWalletProxyResponse.Builder builder) throws Exception {
		byte[] nvram = null;
		if (session != null) {
			ledgerService.close(session);
		}
		ServiceResult result = ledgerService.openDefault();
		filterFailure("openDefault", result);
		session = result.getResult();
		if (openSession.hasNvram()) {
			nvram = openSession.getNvram().toByteArray();
		}
		result = ledgerService.initStorage(session, nvram);
		filterFailure("initStorage", result);
		if (openSession.hasAttestation()) {
			byte[] attestation = openSession.getAttestation().toByteArray();
			result = ledgerService.initAttestation(session, attestation);
			filterFailure("initAttestation", result);
		}
		builder.setGenericAck(GenericAck.newBuilder().build());
	}
	
	private void handleCloseSession(CloseSession closeSession, LedgerWalletProxyResponse.Builder builder) throws Exception {
		if (session != null) {
			ServiceResult result = ledgerService.close(session);
			filterFailure("close", result);			
			session = null;
		}
		builder.setGenericAck(GenericAck.newBuilder().build());
	}
	
	private void writeUint32BE(ByteArrayOutputStream out, int data) {
		out.write((data >> 24) & 0xff);
		out.write((data >> 16) & 0xff);
		out.write((data >> 8) & 0xff);
		out.write(data & 0xff);
	}
	
	private void handleCodeResult(ServiceResult result, LedgerWalletProxyResponse.Builder builder) throws Exception {
		if (result.getExtendedResult() != null) {
			byte[] extendedResult = result.getExtendedResult();
			switch(extendedResult[0]) {
				case STATUS_CODERUNTIME_OK: {
					StartCodeResponseAck.Builder builderResponse = StartCodeResponseAck.newBuilder();
					builderResponse.setResponse(ByteString.copyFrom(extendedResult, 1, extendedResult.length - 1));
					builder.setStartCodeResponseAck(builderResponse.build());
				}					
				break;
				
				case STATUS_CODERUNTIME_LOG: {
					LogAck.Builder builderResponse = LogAck.newBuilder();
					String logMessage = new String(extendedResult, 1, extendedResult.length - 1);
					builderResponse.setMessage(logMessage);
					builder.setLogAck(builderResponse.build());
					logger.logInfo(logMessage);
					logPending = true;
				}
				break;
				
				default: {
					StartCodeErrorAck.Builder builderResponse = StartCodeErrorAck.newBuilder();
					builderResponse.setErrorCode(extendedResult[0] - STATUS_CODERUNTIME_ERROR);
					builder.setStartCodeErrorAck(builderResponse.build());
				}
				break;
			}
		}
		else {
			throw new RuntimeException("Invalid response");
		}		
	}
	
	private void handleStartCode(StartCode startCode, LedgerWalletProxyResponse.Builder builder) throws Exception {
		if (session == null) {
			throw new RuntimeException("No open session");
		}
		int sizeReserved = 0;
		byte[] parameters = null;
		byte[] signature = startCode.getSignature().toByteArray();
		if (startCode.hasParameters()) {
			parameters = startCode.getParameters().toByteArray();
		}
		ByteArrayOutputStream message = new ByteArrayOutputStream();
		// Prepare
		for (CodeRangeType codeRange : startCode.getCodeList()) {
			sizeReserved += codeRange.getEnd() - codeRange.getStart();
			sizeReserved += codeRange.getDataLength();
		}
		sizeReserved += startCode.getStackSize();
		message.write(MSG_CODERUNTIME_INIT);
		writeUint32BE(message, sizeReserved);
		ServiceResult result = ledgerService.exchangeExtended(session, PROTOCOL_CODERUNTIME, message.toByteArray(), null);
		filterFailure("codeRuntime_init", result);
		// Load
		for (CodeRangeType codeRange : startCode.getCodeList()) {
			message = new ByteArrayOutputStream();
			message.write(MSG_CODERUNTIME_LOAD_SECTION);
			message.write(codeRange.getFlags());
			writeUint32BE(message, codeRange.getStart());
			writeUint32BE(message, codeRange.getEnd());
			writeUint32BE(message, codeRange.getDataLength());
			result = ledgerService.exchangeExtended(session, PROTOCOL_CODERUNTIME, message.toByteArray(), codeRange.getData().toByteArray());
			filterFailure("codeRuntime_loadSection", result);				
		}		
		// Run
		message = new ByteArrayOutputStream();
		message.write(MSG_CODERUNTIME_RUN_CODE);
		writeUint32BE(message, startCode.getEntryPoint());
		writeUint32BE(message, startCode.getStackSize());
		if (ui != null) {
			writeUint32BE(message, ui.length);
		}
		else {
			writeUint32BE(message, 0);
		}
		if (parameters != null) {
			writeUint32BE(message, parameters.length);
		}
		else {
			writeUint32BE(message, 1);
		}
		writeUint32BE(message, signature.length);
		message.write(signature);
		int offset = 0;
		Arrays.fill(responseBuffer, (byte)0);
		if (ui != null) {
			System.arraycopy(ui, 0, responseBuffer, offset, ui.length);
			offset += ui.length;
		}
		if (parameters != null) {
			System.arraycopy(parameters, 0, responseBuffer, offset, parameters.length);
		}
		else {
			responseBuffer[offset] = 0;
		}
		result = ledgerService.exchangeExtended(session, PROTOCOL_CODERUNTIME, message.toByteArray(), responseBuffer);
		filterFailure("coderuntime_run", result);
		handleCodeResult(result, builder);
	}
	
	private void handleResumeCode(ResumeCode resumeCode, LedgerWalletProxyResponse.Builder builder) throws Exception {
		if (session == null) {
			throw new RuntimeException("No open session");
		}
		if (!logPending) {
			throw new RuntimeException("No pending log");
		}
		logPending = false;
		ByteArrayOutputStream message = new ByteArrayOutputStream();
		message.write(MSG_CODERUNTIME_RESUME_CODE);
		ServiceResult result = ledgerService.exchangeExtended(session, PROTOCOL_CODERUNTIME, message.toByteArray(), responseBuffer);
		filterFailure("coderuntime_resume", result);
		handleCodeResult(result, builder);
	}
	
	private void handleGetNVRAM(GetNVRAM getNVRAM, LedgerWalletProxyResponse.Builder builder) throws Exception {
		if (session == null) {
			throw new RuntimeException("No open session");
		}
		ServiceResult result = ledgerService.getStorage(session);
		filterFailure("getStorage", result);
		GetNVRAMAck.Builder builderResponse = GetNVRAMAck.newBuilder();
		builderResponse.setNvram(ByteString.copyFrom(result.getResult()));
		builder.setGetNVRAMAck(builderResponse.build());
	}

	private void handleGetAttestation(GetAttestation getAttestation, LedgerWalletProxyResponse.Builder builder) throws Exception {
		if (session == null) {
			throw new RuntimeException("No open session");
		}
		ServiceResult result = ledgerService.getAttestation(session);
		filterFailure("getAttestation", result);
		GetAttestationAck.Builder builderResponse = GetAttestationAck.newBuilder();
		builderResponse.setAttestation(ByteString.copyFrom(result.getResult()));
		builder.setGetAttestationAck(builderResponse.build());
	}
	
	private void handleExchangeWallet(ExchangeWallet exchangeWallet, LedgerWalletProxyResponse.Builder builder) throws Exception {
		if (session == null) {
			throw new RuntimeException("No open session");
		}
		ServiceResult result = ledgerService.exchangeExtendedUI(session, exchangeWallet.getApdu().toByteArray());
		filterFailure("exchange", result);
		ExchangeWalletAck.Builder builderResponse = ExchangeWalletAck.newBuilder();
		builderResponse.setResponse(ByteString.copyFrom(result.getResult()));
		builder.setExchangeWalletAck(builderResponse.build());
	}
		
	private synchronized void handleConnection(Socket connection) {
		try {
			InputStream is = connection.getInputStream();
			OutputStream os = connection.getOutputStream();
			LedgerWalletProxyResponse.Builder builder = LedgerWalletProxyResponse.newBuilder();
			if (busy) {
				// If busy, terminate immediately				
				builder.setBusyAck(BusyAck.newBuilder().build());
				byte[] data = builder.build().toByteArray();
				writeSize(os, data.length);
				os.write(data);
				os.flush();
				os.close();
				is.close();
				try {
					connection.close();
				}
				catch(Exception e) {					
				}
				return;
			}
			session = null;
			logPending = false;
			for (;;) {				
				busy = true;
				int size = readSize(is);
				byte[] dataBuffer = readBuffer(is, size);
				LedgerWalletProxyRequest request = LedgerWalletProxyRequest.parseFrom(dataBuffer);
				try {
					if (request.hasOpenSession()) {
						handleOpenSession(request.getOpenSession(), builder);
					}
					else
					if (request.hasCloseSession()) {
						handleCloseSession(request.getCloseSession(), builder);
					}
					else
					if (request.hasStartCode()) {
						handleStartCode(request.getStartCode(), builder);
					}
					else
					if (request.hasResumeCode()) {
						handleResumeCode(request.getResumeCode(), builder);
					}
					else
					if (request.hasGetNVRAM()) {
						handleGetNVRAM(request.getGetNVRAM(), builder);
					}
					else
					if (request.hasGetAttestation()) {
						handleGetAttestation(request.getGetAttestation(), builder);
					}
					else
					if (request.hasExchangeWallet()) {
						handleExchangeWallet(request.getExchangeWallet(), builder);
					}
					else {
						GenericErrorAck.Builder errorBuilder = GenericErrorAck.newBuilder();
						errorBuilder.setReason("Unsupported request");
						builder.setGenericErrorAck(errorBuilder.build());					
					}
				}
				catch(Exception e) {
					Log.d(MainActivity.TAG, "Exception handling message", e);
					GenericErrorAck.Builder errorBuilder = GenericErrorAck.newBuilder();
					if (e.getMessage() != null) {
						errorBuilder.setReason(e.getMessage());
					}
					else {
						errorBuilder.setReason("");
					}
					builder.setGenericErrorAck(errorBuilder.build());										
				}
				byte[] data = builder.build().toByteArray();
				writeSize(os, data.length);
				os.write(data);
				builder = LedgerWalletProxyResponse.newBuilder();
			}
			
		}
		catch(Exception e) {
			Log.d(MainActivity.TAG, "Exception handling client", e);
			closeSession();
			try {
				connection.close();
			}
			catch(Exception e1) {				
			}
		}
	}
	
	public void stopServer() {
		try {
			logger.logInfo("Stopping server");
			serverSocket.close();
		}
		catch(Exception e) {			
		}
	}
	
    private static Vector<String> getIPAddresses() {
    	Vector<String> result = new Vector<String>();
        try {
            List<NetworkInterface> interfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface intf : interfaces) {
                List<InetAddress> addrs = Collections.list(intf.getInetAddresses());
                for (InetAddress addr : addrs) {
                    if (!addr.isLoopbackAddress()) {
                        result.add(addr.getHostAddress());
                    }
                }
            }
        } 
        catch (Exception e) 
        {         	
        } 
        return result;
    }
	
	
	public void run() {
		logger.logInfo("Starting server");
		Vector<String> addresses = getIPAddresses();
		for (String address : addresses) {
			logger.logInfo("Listening on " + address + ":" + port);
		}
		while (!serverSocket.isClosed()) {
			try {
				Socket connection = serverSocket.accept();
				logger.logInfo("Client connected");
				handleConnection(connection);
				logger.logInfo("Client disconnected");
			}
			catch(Exception e) {
				Log.d(MainActivity.TAG, "Error establishing connection", e);
			}
		}
		logger.logInfo("Server stopped");
	}
	
	

}
