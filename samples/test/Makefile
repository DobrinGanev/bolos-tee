all:	test.aes test.des test.ecdh test.ecdsa test.endorsement_init test.endorsement_run test.hash test.hmac test.init test.pbkdf2 test.rsa test.schnorr test.sodium  test.ui test.wallet test.wrapping

%:	%.c
	moxiebox-gcc -O2 -mel $< -I../../api -L../../runtime -lbolos -o $@

clean:
	rm -rf test.aes test.des test.ecdh test.ecdsa test.endorsement_init test.endorsement_run test.hash test.hmac test.init test.pbkdf2 test.rsa test.schnorr test.sodium test.ui test.wallet test.wrapping

