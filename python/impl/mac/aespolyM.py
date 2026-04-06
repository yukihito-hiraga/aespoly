from impl.common.utils import partition, ozp
from Crypto.Util.strxor import strxor
from impl.blockcipher.protocol import BlockCipher
from impl.hash.protocol import TweakableHash
from impl.specs import Field, SchemeSpec

class AESpolyM:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
            f"aespolyM_{E.spec().name}",
            [
                Field("pt", E.blocksize(), None),
                Field("key1", E.blocksize(), E.blocksize()),
                Field("key2", E.blocksize(), E.blocksize()),
                Field("h", E.blocksize(), E.blocksize()),
            ],
            ["tag"],
            [],
        )
    
    def __init__(self, cipher1:BlockCipher, cipher2:BlockCipher, H:TweakableHash):
        self.cipher1 = cipher1
        self.cipher2 = cipher2
        assert cipher1.blocksize() == cipher2.blocksize()
        self.blocksize = cipher1.blocksize()
        self.H = H
        
    def hash(self, pt: bytes):
        M = ozp(pt, self.blocksize)
        m = len(M) // self.blocksize
        if m % 2 == 0:
            M += (0).to_bytes(self.blocksize)
            m += 1
        D = b''
        Ms = partition(M, self.blocksize)
        for i in range(1, (m-1)//2):
            D += strxor(Ms[2*i-2], self.cipher1.encrypt(Ms[2*i-1]))
        V = self.H.hash(b'', D)
        T = self.cipher2.encrypt(strxor(V, Ms[m-1]))
        return T