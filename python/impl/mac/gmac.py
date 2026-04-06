from impl.ae.gcm import GCM
from impl.blockcipher.protocol import BlockCipher
from impl.specs import SchemeSpec, Field

class GMAC:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
            f"gmac_{E.spec().name}",
            [
                Field("iv", 0, None),
                Field("ad", 0, None),
                Field("key", 16, 16),
            ],
            ["tag"],
            [
                {
                    "scheme" : f"gmac_{E.spec().name}",
                    "inputs": {
                        "key": "FEFFE9928665731C6D6A8F9467308308",
                        "iv": "CAFEBABEFACEDBADDECAF888",
                        "ad": "3AD77BB40D7A3660A89ECAF32466EF97"
                        "F5D3D58503B9699DE785895A96FDBAAF"
                        "43B1CD7F598ECE23881B00E3ED030688"
                        "7B0C785E27E8AD3F8223207104725DD4",
                    },
                    "expected": {
                        "tag": "5F91D77123EF5EB9997913849B8DC1E9",
                    },
                },
            ],
        )

    def __init__(self, cipher: BlockCipher):
        self.n = cipher.blocksize()
        assert self.n == 16
        self.gcm = GCM(cipher)

    def hash(self, iv: bytes, ad: bytes):
        return self.gcm.encrypt(iv, ad, b'')[1]