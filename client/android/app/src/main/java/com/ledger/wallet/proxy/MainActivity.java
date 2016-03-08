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

import java.text.SimpleDateFormat;
import java.util.Calendar;

import com.ledger.wallet.service.ILedgerWalletService;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

@SuppressLint("NewApi")
public class MainActivity extends AppCompatActivity implements InfoLogger {
	
	public static final String KEY_INTENT_PORT = "port";
	
	protected static final String TAG = "LedgerWalletProxy";
		
	private static final String LEDGER_SERVICE_CLASS_NAME = "com.ledger.wallet.service";
	private static final int LEDGER_SERVICE_VERSION_CODE = 2;
	
	private static final int OPTIONS = 1;
	
    private TextView mInfoTextView;
    private Button mStartStopButton;
    private Button mOptionsButton;
    
    private ILedgerWalletService ledgerService;
    //private SharedPreferences sharedPreferences;
    private ProxyServer server;
    private int port;
    private int defaultPort;
    
	
	ServiceConnection ledgerServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
        	ledgerService = ILedgerWalletService.Stub.asInterface(service);
        	logInfo("Service connected");
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
        	ledgerService = null;
        	logInfo("Service disconnected");
        	if (server != null) {
        		server.stopServer();
        		server.interrupt();
        		server = null;        		
        	}
        	connectService();
        }
		
	};
		
	private boolean isServiceInstalled() {
		PackageManager pm = getPackageManager();
		boolean installed = false;
		try {
			PackageInfo packageInfo = pm.getPackageInfo(LEDGER_SERVICE_CLASS_NAME, 0);
			installed = packageInfo.versionCode == LEDGER_SERVICE_VERSION_CODE;
		}
		catch(Exception e) {			
		}
		return installed;
	}
	
	@Override
	public void logInfo(final String message) {
		runOnUiThread(new Runnable() {
			@Override
			public void run() {
				String time =  new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(Calendar.getInstance().getTime());
				mInfoTextView.append("[" + time + "] " + message + "\n");				
			}			
		});
	}
	
	private void connectService() {
		Intent intent = new Intent(ILedgerWalletService.class.getName());
		intent.setPackage(LEDGER_SERVICE_CLASS_NAME);        	
		boolean result = bindService(intent, ledgerServiceConnection, Context.BIND_AUTO_CREATE);
		if (!result) {
			Toast.makeText(MainActivity.this, R.string.service_connect_error, Toast.LENGTH_LONG).show();
			finish();
		}		
	}
		   
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        defaultPort = Integer.parseInt(getResources().getString(R.string.default_port_text));
        port = defaultPort;
        //sharedPreferences = this.getPreferences(Context.MODE_PRIVATE);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);        
        setSupportActionBar(toolbar);
        mInfoTextView = (TextView) findViewById(R.id.info_text);
        mStartStopButton = (Button) findViewById(R.id.startstop_button);
        mOptionsButton = (Button) findViewById(R.id.options_button);
        mStartStopButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            	if (server == null) {
            		if (ledgerService != null) {
            			mInfoTextView.setText("");
            			server = ProxyServer.create(MainActivity.this, ledgerService, port);
            			server.start();
            		}
            		else {
                    	Toast.makeText(MainActivity.this, R.string.service_not_ready, Toast.LENGTH_LONG).show();
            		}
            	}
            	else {
            		server.stopServer();
            		server.interrupt();
            		server = null;
            	}
            }
        });

        mOptionsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            	Intent intent = new Intent(MainActivity.this, OptionsActivity.class);
            	startActivityForResult(intent, OPTIONS);
            }
        });                
        TypefaceHelper.applyFonts((ViewGroup) getWindow().getDecorView());
        
        if (isServiceInstalled()) {
        	if (ledgerService == null) {
        		connectService();
        	}
        }
        else {
        	Toast.makeText(MainActivity.this, R.string.service_not_installed, Toast.LENGTH_LONG).show();
        	finish();
        }
    }
    
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == OPTIONS) {
			if (resultCode == RESULT_OK) {
				port = data.getIntExtra(KEY_INTENT_PORT, defaultPort);
			}
		}
	}
    
}
