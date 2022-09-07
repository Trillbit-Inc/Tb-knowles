import sys
import os
import base64
import json

sys.path.append("../chacha20-poly1305")

from chacha20poly1305 import encrypt_and_tag

# as used on board side
#DVK = bytes([0x5d, 0x4, 0xdc, 0x53, 0xbd, 0x46, 0x74, 0xa1, 0x0, 0x5e, 0x1, 0xc3, 0x86, 0xdd, 0xf9, 0x45, 0xdc, 0x6d, 0xea, 0xb0, 0x10, 0x2b, 0x8b, 0x6d, 0xe, 0xe7, 0x49, 0x64, 0x64, 0x8b, 0xa0, 0x52])
DVK = bytes([180, 197, 87, 99, 182, 107, 197, 228, 80, 101, 197, 118, 10, 3, 35, 226, 204, 172, 129, 17, 183, 80, 20, 235, 141, 24, 47, 26, 137, 161, 195, 2])

NONCE_SIZE = 12
KEY_SIZE = 32

"""
Fixed CK and CK Nonce for demo.
"""
FIXED_CK = bytes([x for x in range(KEY_SIZE)])
FIXED_CK_NONCE = bytes([x for x in range(NONCE_SIZE)])


def create_license(lic_params):
    plainbytes = json.dumps(lic_params, separators=(',', ':')).encode('utf-8')
    #print("DVC plain text json:", plainbytes)
    nonce = os.urandom(NONCE_SIZE)
    aad = b'' # not used
    cipher, tag = encrypt_and_tag(DVK, nonce, plainbytes, aad)
    dvc_bytes = cipher + tag
    license = nonce + dvc_bytes
    b64_license = base64.b64encode(license)
    return b64_license

def create_ck():
    key = FIXED_CK #os.urandom(KEY_SIZE)
    nonce = FIXED_CK_NONCE #os.urandom(NONCE_SIZE)
    b64_key = base64.b64encode(key)
    b64_key_nonce = base64.b64encode(nonce)
    return (b64_key, b64_key_nonce)

