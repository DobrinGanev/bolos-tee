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

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

@SuppressLint("NewApi")
public class OptionsActivity extends AppCompatActivity {

    private EditText mPortText;
    private Button mSubmitButton;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_options);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setHomeButtonEnabled(true);
        getSupportActionBar().setDisplayShowHomeEnabled(true);

        getSupportActionBar().setHomeAsUpIndicator(R.drawable.abc_ic_ab_back_mtrl_am_alpha);

        mPortText = (EditText) findViewById(R.id.port_edittext);
        mSubmitButton = (Button) findViewById(R.id.submit_button);

        mSubmitButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            	int port = 0;
            	try {
            		port = Integer.parseInt(mPortText.getText().toString());
            	}
            	catch(Exception e) {            		
            	}
            	if (port == 0) {
            		Toast.makeText(OptionsActivity.this, R.string.options_message_invalid_port, Toast.LENGTH_LONG).show();            		
            	}
            	else {
            		Intent intent = getIntent();
            		intent.putExtra(MainActivity.KEY_INTENT_PORT, Integer.parseInt(mPortText.getText().toString()));
            		setResult(RESULT_OK, intent);
            		finish();
            	}
            }
        });

        TypefaceHelper.applyFonts((ViewGroup) getWindow().getDecorView());
    }

}
