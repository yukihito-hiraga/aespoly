import pytest

from random import randbytes
from impl.blockcipher.aes import AES128

def test_NIST():
	key = bytes.fromhex("2B7E151628AED2A6ABF7158809CF4F3C")
	aes128 = AES128(key)
	pt1 = bytes.fromhex("6BC1BEE22E409F96E93D7E117393172A")
	pt2 = bytes.fromhex("AE2D8A571E03AC9C9EB76FAC45AF8E51")
	pt3 = bytes.fromhex("30C81C46A35CE411E5FBC1191A0A52EF")
	pt4 = bytes.fromhex("F69F2445DF4F9B17AD2B417BE66C3710")
	ct1 = aes128.encrypt(pt1)
	ct2 = aes128.encrypt(pt2)
	ct3 = aes128.encrypt(pt3)
	ct4 = aes128.encrypt(pt4)
	assert ct1 == bytes.fromhex("3AD77BB40D7A3660A89ECAF32466EF97")
	assert ct2 == bytes.fromhex("F5D3D58503B9699DE785895A96FDBAAF")
	assert ct3 == bytes.fromhex("43B1CD7F598ECE23881B00E3ED030688")
	assert ct4 == bytes.fromhex("7B0C785E27E8AD3F8223207104725DD4")
 
def test_NIST_inv():
	key = bytes.fromhex("2B7E151628AED2A6ABF7158809CF4F3C")
	aes128 = AES128(key)
	ct1 = bytes.fromhex("3AD77BB40D7A3660A89ECAF32466EF97")
	ct2 = bytes.fromhex("F5D3D58503B9699DE785895A96FDBAAF")
	ct3 = bytes.fromhex("43B1CD7F598ECE23881B00E3ED030688")
	ct4 = bytes.fromhex("7B0C785E27E8AD3F8223207104725DD4")
	pt1 = aes128.decrypt(ct1)
	pt2 = aes128.decrypt(ct2)
	pt3 = aes128.decrypt(ct3)
	pt4 = aes128.decrypt(ct4)
	assert pt1 == bytes.fromhex("6BC1BEE22E409F96E93D7E117393172A")
	assert pt2 == bytes.fromhex("AE2D8A571E03AC9C9EB76FAC45AF8E51")
	assert pt3 == bytes.fromhex("30C81C46A35CE411E5FBC1191A0A52EF")
	assert pt4 == bytes.fromhex("F69F2445DF4F9B17AD2B417BE66C3710")
 
def test_random_inv():
    key = randbytes(16)
    pt = randbytes(16)
    aes = AES128(key)
    ct = aes.encrypt(pt)
    ppt = aes.decrypt(ct)
    assert ppt == pt