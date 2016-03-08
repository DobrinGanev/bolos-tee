#!/bin/bash
dd if=/dev/zero of=/tmp/zero bs=1 count=100 >/dev/null 2>/dev/null
moxiebox-objcopy --add-section .ledger=/tmp/zero $1
./signMoxie $1 /tmp/signature.bin
moxiebox-objcopy --update-section .ledger=/tmp/signature.bin $1
rm /tmp/zero
rm /tmp/signature.bin

