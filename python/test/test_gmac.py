import pytest

from impl.blockcipher.aes import AES128
from impl.mac.gmac import GMAC

def test_NIST_aes128_1():
	key = bytes.fromhex("00000000000000000000000000000000")
	aes128 = AES128(key)
	gmac = GMAC(aes128)
	iv = bytes.fromhex("000000000000000000000000")
	ad = bytes.fromhex("")
	tag = gmac.hash(iv, ad)
	assert tag == bytes.fromhex("58e2fccefa7e3061367f1d57a4e7455a")