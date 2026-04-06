from impl.common.utils import sumxor, ozp
from impl.xex import XE
from Crypto.Util.strxor import strxor
from math import ceil
from impl.blockcipher.protocol import BlockCipher
from impl.specs import SchemeSpec, Field

class PMAC:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
            f"pmac_{E.spec().name}",
            [
                Field("pt", 0, None),
                Field("key", 16, 16),
            ],
            ["tag"],
            [
                {
                    "scheme" : f"gmac_{E.spec().name}",
                    "inputs": {
                        "key": "000102030405060708090a0b0c0d0e0f",
                        "pt": "",
                    },
                    "expected": {
                        "tag": "4399572cd6ea5341b8d35876a7098af7",
                    },
                },
            ],
        )
    
    def half(self, x:bytes):
        assert len(x) == self.blocksize
        r = (0).to_bytes(self.blocksize)
        y = int.from_bytes(x, 'big')
        #print("{:x}".format(y))
        if (y & (1 << (self.blocksize*8-1))) != 0:
            r = sum(1 << i for i in [127, 6 , 1, 0]).to_bytes(self.blocksize, 'big')
            #print("r", r.hex())
        return strxor((y >> 1).to_bytes(self.blocksize, 'big'), r)

    def __init__(self, cipher:BlockCipher):
        self.blocksize = cipher.blocksize()
        self.cipher = cipher
        L = self.L = cipher.encrypt((0).to_bytes(self.blocksize))
        self.xe = XE(cipher, 0x87, L)
        self.Lhalf = self.half(L)
        
    def hash(self, pt:bytes):
        m = max(1, ceil(len(pt) / self.blocksize))
        rem = len(pt) - (m-1) * self.blocksize
        ct, _ = self.xe.encrypt_graycode(pt[:-rem])
        h = sumxor(ct, self.blocksize)
        S = strxor(h, ozp(pt[-rem:], self.blocksize))
        
        if rem == self.blocksize:
            S = strxor(S, self.Lhalf)
        return self.cipher.encrypt(S)
