import pytest

from impl.blockcipher.aes import AES128
from impl.ae.ocb import OCB

def test_RFC_aes128_1():
	key = bytes.fromhex("000102030405060708090A0B0C0D0E0F")
	aes128 = AES128(key)
	ocb = OCB(aes128)
	nonce = bytes.fromhex("BBAA99887766554433221100")
	pt = bytes.fromhex("")
	ad = bytes.fromhex("")
	ct = ocb.encrypt(nonce, ad, pt)
	assert ct  == bytes.fromhex("785407BFFFC8AD9EDCC5520AC9111EE6")
 
def test_RFC_aes128_2():
	key = bytes.fromhex("000102030405060708090A0B0C0D0E0F")
	aes128 = AES128(key)
	ocb = OCB(aes128)
	nonce = bytes.fromhex("BBAA99887766554433221101")
	pt = bytes.fromhex("0001020304050607")
	ad = bytes.fromhex("0001020304050607")
	ct = ocb.encrypt(nonce, ad, pt)
	assert ct  == bytes.fromhex("6820B3657B6F615A5725BDA0D3B4EB3A257C9AF1F8F03009")
 
def test_RFC_aes128_3():
	key = bytes.fromhex("000102030405060708090A0B0C0D0E0F")
	aes128 = AES128(key)
	ocb = OCB(aes128)
	nonce = bytes.fromhex("BBAA99887766554433221102")
	pt = bytes.fromhex("")
	ad = bytes.fromhex("0001020304050607")
	ct = ocb.encrypt(nonce, ad, pt)
	assert ct  == bytes.fromhex("81017F8203F081277152FADE694A0A00")
 
def test_RFC_aes128_4():
	key = bytes.fromhex("000102030405060708090A0B0C0D0E0F")
	aes128 = AES128(key)
	ocb = OCB(aes128)
	nonce = bytes.fromhex("BBAA99887766554433221103")
	pt = bytes.fromhex("0001020304050607")
	ad = bytes.fromhex("")
	ct = ocb.encrypt(nonce, ad, pt)
	assert ct  == bytes.fromhex("45DD69F8F5AAE72414054CD1F35D82760B2CD00D2F99BFA9")