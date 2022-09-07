
import math
import binascii

def clamp(r: int) -> int:
    return r & 0x0ffffffc0ffffffc0ffffffc0fffffff

def le_bytes_to_num(byte) -> int:
    res = 0
    for i in range(len(byte) - 1, -1, -1):
        res <<= 8
        res += byte[i]
    return res

def num_to_16_le_bytes(num: int) -> bytes:
    res = []
    for i in range(16):
        res.append(num & 0xff)
        num >>= 8
    return bytearray(res)

def poly1305_mac(msg: bytes, key: bytes) -> bytes:
    r = le_bytes_to_num(key[0:16])
    r = clamp(r)
    s = le_bytes_to_num(key[16:32])
    a = 0  # a is the accumulator
    p = (1<<130) - 5
    for i in range(1, math.ceil(len(msg)/16) + 1):
        n = le_bytes_to_num(msg[(i-1)*16 : i*16] + b'\x01')
        a += n
        a = (r * a) % p
    a += s
    return num_to_16_le_bytes(a)
