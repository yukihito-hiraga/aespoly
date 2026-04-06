from impl.common.utils import partition
from impl.blockcipher.protocol import BlockCipher
from Crypto.Util.strxor import strxor
from functools import reduce

class XCTR:
    def __init__(self, cipher : BlockCipher):
        self.blocksize = cipher.blocksize()
        self.cipher = cipher

    def encrypt(self, S: bytes, pt: bytes, verbose=False) -> bytes:
        cipher = self.cipher
        assert len(S) >= self.blocksize
        S = S[:self.blocksize]
        ct = b''
        for i, X in enumerate(partition(pt, self.blocksize)):
            ctr = i + 1
            pr = cipher.encrypt(
                strxor(ctr.to_bytes(self.blocksize, 'little'), S))
            if verbose:
                print(strxor(X, pr).hex())
            ct = ct + strxor(X, pr)
        n = len(pt)//self.blocksize+1
        rem = len(pt) % self.blocksize
        if rem > 0:
            ct = ct + \
                strxor(
                    pt[-rem:], cipher.encrypt(strxor(n.to_bytes(self.blocksize, 'little'), S))[:rem])
        return ct
