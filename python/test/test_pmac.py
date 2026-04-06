import pytest

from impl.blockcipher.aes import AES128
from impl.mac.pmac import PMAC

def test_rogaway_aes128_1():
	key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
	aes128 = AES128(key)
	pmac = PMAC(aes128)
	pt = bytes.fromhex("")
	tag = pmac.hash(pt)
	assert tag == bytes.fromhex("4399572cd6ea5341b8d35876a7098af7")
 
def test_rogaway_aes128_2():
	key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
	aes128 = AES128(key)
	pmac = PMAC(aes128)
	pt = bytes.fromhex("000102")
	tag = pmac.hash(pt)
	assert tag == bytes.fromhex("256ba5193c1b991b4df0c51f388a9e27")
 
def test_rogaway_aes128_3():
	key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
	aes128 = AES128(key)
	pmac = PMAC(aes128)
	pt = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
	tag = pmac.hash(pt)
	assert tag == bytes.fromhex("ebbd822fa458daf6dfdad7c27da76338")