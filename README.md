# bolos-tee
Implementation of Ledger BOLOS for TEE applications

Doxygen API available on https://ledgerhq.github.io/bolos-tee/

To be used with Ledger Trustlet application : https://play.google.com/store/apps/details?id=com.ledger.wallet.bootstrap - add your uncompressed secp256k1 public key into "Options" before purchasing.

Building the SDK
=================

  * Build the moxiebox cross compiler toolchain from https://github.com/jgarzik/moxiebox (or wait for a Docker image)

  * Build the signing tool from https://github.com/LedgerHQ/bolos-tee/tree/master/tools/signing

  * Build the BOLOS runtime environment from https://github.com/LedgerHQ/bolos-tee/tree/master/runtime

  * Build and install the Python communication library from https://github.com/LedgerHQ/bolos-tee/tree/master/client/python

Building code
==============

  * Compile your BOLOS code, preferably on a trusted computer, using the API (https://github.com/LedgerHQ/bolos-tee/tree/master/api) and runtime environment - a few examples are provided in https://github.com/LedgerHQ/bolos-tee/tree/master/samples

  * Sign your code on a trusted computer (or write a BOLOS signer)

Running code
=============

  * Build and install Ledger Wallet Proxy (https://github.com/LedgerHQ/bolos-tee/tree/master/client/android) on a development phone on which Ledger Trusted is installed

  * Start a Ledger Wallet Proxy session - make sure the phone is on the same network than the computer used for development

  * Load and execute the code using the Python API (as an example you can refer to https://github.com/LedgerHQ/bolos-tee/blob/master/samples/ethereum/test_ethereum.py)

Contact
=======

Please report bugs and features to hello@ledger.fr



