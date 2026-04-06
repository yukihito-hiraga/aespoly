import pytest

from random import randbytes
from impl.blockcipher.rijndael256 import Rijndael256

def test_TheDesignofRijndael():
	key = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
	pt  = bytes.fromhex("C6227E7740B7E53B5CB77865278EAB0726F62366D9AABAD908936123A1FC8AF3")
	rijndael256 = Rijndael256(key)
	ct = rijndael256.encrypt(pt)
	assert ct == bytes.fromhex("9843E807319C32AD1EA3935EF56A2BA96E4BF19C30E47D88A2B97CBBF2E159E7")
 
def test_TheDesignofRijndael_inv():
	key = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
	ct  = bytes.fromhex("9843E807319C32AD1EA3935EF56A2BA96E4BF19C30E47D88A2B97CBBF2E159E7")
	rijndael256 = Rijndael256(key, True)
	pt = rijndael256.decrypt(ct)
	assert pt == bytes.fromhex("C6227E7740B7E53B5CB77865278EAB0726F62366D9AABAD908936123A1FC8AF3")
 
def test_random_inv():
    key = randbytes(32)
    pt = randbytes(32)
    rijndael256 = Rijndael256(key)
    ct = rijndael256.encrypt(pt)
    ppt = rijndael256.decrypt(ct)
    assert ppt == pt