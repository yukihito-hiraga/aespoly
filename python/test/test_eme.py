import pytest

from impl.blockcipher.aes import AES128
from impl.wbe.eme import EME
from impl.common.utils import strxor

def test_handmade_aes128_1():
	key = bytes.fromhex("00000000000000000000000000000000")
	aes128 = AES128(key)
	eme = EME(aes128)
	pt = bytes.fromhex("00000000000000000000000000000000")
	tweak = bytes.fromhex("00000000000000000000000000000000")
	ct = eme.encrypt(tweak, pt, True)
 
	L = bytes.fromhex("66e94bd4ef8a2c3b884cfa59ca342b2e")
	CC1 = bytes.fromhex("47c78395e0d8ae2194da0a90abc9888a")

	assert ct  == strxor(L, CC1)
