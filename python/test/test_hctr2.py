import pytest

from impl.blockcipher.aes import AES128
from impl.wbe.hctr2 import HCTR2

def test_google_aes128_1():
	key = bytes.fromhex("74f98f60786abfa85b0bbba059e0f91e")
	aes128 = AES128(key)
	hctr2 = HCTR2(aes128)
	pt = bytes.fromhex("6b26837bdc1c583dc142c6ab7b3f43b0")
	tweak = bytes.fromhex("")
	ct = hctr2.encrypt(tweak, pt)
	assert ct  == bytes.fromhex("dd05a8ae51f1e8212fd6c33b9467036d")
