#!/usr/bin/env python3
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from binascii import *
import os

key = unhexlify("4044e65bf1ba4f8c4db767fa6c63e327")
nonce = unhexlify("cb72d71df3c953aaec3ca4d75b")

def test(hdr, clrmsg):
    global nonce

    cipher = AES.new(key, AES.MODE_CCM, nonce, mac_len = 8)
    cipher.update(hdr)

    # Encrypt
    ciphertext = cipher.encrypt(clrmsg)
    mac = cipher.digest()

    # Vector with correct auth
    print("{ \"%s\"," %(hexlify(hdr)))
    print("  \"%s\"," %(hexlify(clrmsg)))
    print("  \"%s\" }," %(hexlify(hdr + ciphertext + mac)))

    # Vector with corrupted auth
    print("{ NULL,")
    print("  NULL,")
    print("  \"%s\" }," %(hexlify(hdr + ciphertext + os.urandom(8))))

test(os.urandom(0), os.urandom(0))
test(os.urandom(0), os.urandom(20))
test(os.urandom(8), os.urandom(173))
test(os.urandom(8), os.urandom(255))
test(os.urandom(8), os.urandom(256))
test(os.urandom(8), os.urandom(344))
test(os.urandom(8), os.urandom(1200))
test(os.urandom(255), os.urandom(0))
test(os.urandom(256), os.urandom(0))
test(os.urandom(1200), os.urandom(0))
test(os.urandom(1200), os.urandom(1200))
test(os.urandom(0xfeff), os.urandom(0xffff))
