import pytest

from impl.blockcipher.aes import AES128
from impl.ae.gcm import GCM

def test_NIST_aes128_1():
	key = bytes.fromhex("00000000000000000000000000000000")
	aes128 = AES128(key)
	gcm = GCM(aes128)
	iv = bytes.fromhex("000000000000000000000000")
	pt = bytes.fromhex("")
	ad = bytes.fromhex("")
	ct, tag = gcm.encrypt(iv, ad, pt)
	assert ct  == bytes.fromhex("")
	assert tag == bytes.fromhex("58e2fccefa7e3061367f1d57a4e7455a")
 
def test_NIST_aes128_2():
	key = bytes.fromhex("00000000000000000000000000000000")
	aes128 = AES128(key)
	gcm = GCM(aes128)
	iv = bytes.fromhex("000000000000000000000000")
	pt = bytes.fromhex("00000000000000000000000000000000")
	ad = bytes.fromhex("")
	ct, tag = gcm.encrypt(iv, ad, pt)
	assert ct  == bytes.fromhex("0388dace60b6a392f328c2b971b2fe78")
	assert tag == bytes.fromhex("ab6e47d42cec13bdf53a67b21257bddf")
 
def test_NIST_aes128_3():
	key = bytes.fromhex("feffe9928665731c6d6a8f9467308308")
	aes128 = AES128(key)
	gcm = GCM(aes128)
	iv = bytes.fromhex("cafebabefacedbaddecaf888")
	pt = bytes.fromhex("d9313225f88406e5a55909c5aff5269a"
					   "86a7a9531534f7da2e4c303d8a318a72"
					   "1c3c0c95956809532fcf0e2449a6b525"
					   "b16aedf5aa0de657ba637b391aafd255")
	ad = bytes.fromhex("")
	ct, tag = gcm.encrypt(iv, ad, pt)
	assert ct  == bytes.fromhex("42831ec2217774244b7221b784d0d49c"
								"e3aa212f2c02a4e035c17e2329aca12e"
								"21d514b25466931c7d8f6a5aac84aa05"
								"1ba30b396a0aac973d58e091473f5985")
	assert tag == bytes.fromhex("4d5c2af327cd64a62cf35abd2ba6fab4")
 
def test_NIST_aes128_4():
	key = bytes.fromhex("feffe9928665731c6d6a8f9467308308")
	aes128 = AES128(key)
	gcm = GCM(aes128)
	iv = bytes.fromhex("cafebabefacedbaddecaf888")
	pt = bytes.fromhex("d9313225f88406e5a55909c5aff5269a"
					   "86a7a9531534f7da2e4c303d8a318a72"
					   "1c3c0c95956809532fcf0e2449a6b525"
					   "b16aedf5aa0de657ba637b39")
	ad = bytes.fromhex("feedfacedeadbeeffeedfacedeadbeef"
					   "abaddad2")
	ct, tag = gcm.encrypt(iv, ad, pt)
	assert ct  == bytes.fromhex("42831ec2217774244b7221b784d0d49c"
								"e3aa212f2c02a4e035c17e2329aca12e"
								"21d514b25466931c7d8f6a5aac84aa05"
								"1ba30b396a0aac973d58e091")
	assert tag == bytes.fromhex("5bc94fbc3221a5db94fae95ae7121a47")
 
def test_NIST_aes128_5():
	key = bytes.fromhex("feffe9928665731c6d6a8f9467308308")
	aes128 = AES128(key)
	gcm = GCM(aes128)
	iv = bytes.fromhex("cafebabefacedbad")
	pt = bytes.fromhex("d9313225f88406e5a55909c5aff5269a"
					   "86a7a9531534f7da2e4c303d8a318a72"
					   "1c3c0c95956809532fcf0e2449a6b525"
					   "b16aedf5aa0de657ba637b39")
	ad = bytes.fromhex("feedfacedeadbeeffeedfacedeadbeef"
					   "abaddad2")
	ct, tag = gcm.encrypt(iv, ad, pt)
	assert ct  == bytes.fromhex("61353b4c2806934a777ff51fa22a4755"
								"699b2a714fcdc6f83766e5f97b6c7423"
								"73806900e49f24b22b097544d4896b42"
								"4989b5e1ebac0f07c23f4598")
	assert tag == bytes.fromhex("3612d2e79e3b0785561be14aaca2fccb")