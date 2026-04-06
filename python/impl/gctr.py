from impl.common.utils import partition
from Crypto.Util.strxor import strxor
from functools import reduce
from math import ceil
from Crypto.Cipher import AES

def inc32(X:bytes):
	n = len(X)
	return (int.from_bytes(X, 'big') + 1).to_bytes(n)

class GCTR:
	def __init__(self, cipher):
		self.block_size = cipher.blocksize()
		self.cipher = cipher
	
	def encrypt(self, S:bytes, pt:bytes) -> bytes:
		if len(pt) == 0:
			return bytes()
		n = ceil(len(pt) / self.block_size)
		cipher = self.cipher
		assert len(S) >= self.block_size
		S = S[:self.block_size]
		cts = []
		CB = S
		for X in partition(pt, self.block_size):
			pr = cipher.encrypt(CB)
			ct = strxor(X, pr)
			cts.append(ct)
			CB = inc32(CB)
		ct = reduce(lambda s,x:s+x, cts, b'')
		rem = len(pt) % self.block_size
		if rem > 0:
			ct = ct + strxor(pt[-rem:], cipher.encrypt(CB)[:rem])
		return  ct
