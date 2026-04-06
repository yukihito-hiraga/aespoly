from impl.blockcipher.protocol import BlockCipher
from impl.common.utils import pad
from impl.xctr import XCTR
from Crypto.Util.strxor import strxor
from functools import reduce
from impl.hash.polyval import Polyval, DBPolyval
from impl.hash.protocol import TweakableHash
from impl.specs import SchemeSpec, Field

def FIXED(E :type[BlockCipher]):
    return {
        "aes128": [
            {
                "scheme": f"hctr2_{E.spec().name}",
                "inputs": {
                    "key": "74f98f60786abfa85b0bbba059e0f91e",
                    "tweak": "",
                    "pt": "6b26837bdc1c583dc142c6ab7b3f43b0",
                },
                "expected": {
                    "ct": "dd05a8ae51f1e8212fd6c33b9467036d",
                },
            },
            {
                "scheme": f"hctr2_{E.spec().name}",
                "inputs": {
                    "key": "d09431cdb9d16d736d58d5b4e86146e2",
                    "tweak": "6f",
                    "pt": "5d4dc932966d0e3a74537ad3ac662771",
                },
                "expected": {
                    "ct": "3a3416b01ddd84d44e2fdf636e7dc782",
                },
            },
        ]
    }
    
class Hash(TweakableHash):
    @property
    def hashsize(self) -> int:
        return self.size

    def __init__(self, key: bytes):
        self.size = len(key)
        if len(key) > 16:
            self.polyval = DBPolyval(key)
        else:
            self.polyval = Polyval(key)
    
    def hash(self, tweak: bytes, pt: bytes) -> bytes:
        is_divisible = len(pt) % 16 == 0
        LEN = 2 * (8 * len(tweak)) + 2 + (0 if is_divisible else 1)
        lblock = LEN.to_bytes(self.size, 'little')
        return self.polyval.hash( lblock + pad(tweak, self.size) + ( pt if is_divisible else pad(pt + b'\x01', self.size)) )
    
    @staticmethod
    def hash_with_key(key: bytes, tweak: bytes, pt: bytes) -> bytes:
        return Hash(key).hash(tweak, pt)

class HCTR2:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
            f"hctr2_{E.spec().name}",
            [
                Field("pt", E.blocksize(), None),
                Field("tweak", 0, None),
                Field("key", E.blocksize(), E.blocksize()),
            ],
            ["ct"],
            FIXED(E)[E.spec().name] if E.spec().name in FIXED(E).keys() else [],
        )

    def __init__(self, cipher: BlockCipher):
        self.blocksize = cipher.blocksize()
        self.cipher = cipher
        self.xctr = XCTR(cipher)
        self.H = Hash(cipher.encrypt((0).to_bytes(self.blocksize)))

    def encrypt(self, tw, pt, verbose=False):
        h = self.cipher.encrypt((0).to_bytes(self.blocksize, "big"))
        L = self.cipher.encrypt((1).to_bytes(self.blocksize, "little"))
        M = pt[: self.blocksize]
        N = pt[self.blocksize :]
        MM = strxor(M, self.H.hash(tw, N))
        UU = self.cipher.encrypt(MM)
        S = strxor(MM, strxor(UU, L))
        V = self.xctr.encrypt(S, N)
        U = strxor(UU, self.H.hash(tw, V))
        ct = U + V
        if verbose:
            print("h:", h.hex())
            print("L:", L.hex())
            print("H(h, N, tw):", self.H.hash(tw, N).hex())
            print("N:", N.hex())
            print("M:", M.hex())
            print("MM:", MM.hex())
            print("UU:", UU.hex())
            print("S:", S.hex())
        return ct
