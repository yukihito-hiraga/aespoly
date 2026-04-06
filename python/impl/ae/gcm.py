from math import ceil
from impl.hash.ghash import GHASH
from impl.gctr import GCTR, inc32
from impl.blockcipher.protocol import BlockCipher
from impl.specs import SchemeSpec, Field


class GCM:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
            f"gcm_{E.spec().name}",
            [
                Field("iv", 0, None),
                Field("pt", 0, None),
                Field("ad", 0, None),
                Field("key", 16, 16),
            ],
            ["ct", "tag"],
            [
                {
                    "scheme" : f"gcm_{E.spec().name}",
                    "inputs": {
                        "key": "00000000000000000000000000000000",
                        "iv": "000000000000000000000000",
                        "ad": "",
                        "pt": "00000000000000000000000000000000",
                    },
                    "expected": {
                        "ct": "0388dace60b6a392f328c2b971b2fe78",
                        "tag": "ab6e47d42cec13bdf53a67b21257bddf",
                    },
                },
                {
                    "scheme" : f"gcm_{E.spec().name}",
                    "inputs": {
                        "key": "FEFFE9928665731C6D6A8F9467308308",
                        "iv": "CAFEBABEFACEDBADDECAF888",
                        "ad": "3AD77BB40D7A3660A89ECAF32466EF97"
                        "F5D3D58503B9699DE785895A96FDBAAF"
                        "43B1CD7F598ECE23881B00E3ED030688"
                        "7B0C785E27E8AD3F8223207104725DD4",
                        "pt": "",
                    },
                    "expected": {
                        "ct": "",
                        "tag": "5F91D77123EF5EB9997913849B8DC1E9",
                    },
                },
                {
                    "scheme" : f"gcm_{E.spec().name}",
                    "inputs": {
                        "key": "feffe9928665731c6d6a8f9467308308",
                        "iv": "cafebabefacedbaddecaf888",
                        "pt": "d9313225f88406e5a55909c5aff5269a"
							  "86a7a9531534f7da2e4c303d8a318a72"
							  "1c3c0c95956809532fcf0e2449a6b525"
							  "b16aedf5aa0de657ba637b391aafd255",
                        "ad": "",
                    },
                    "expected": {
                        "ct": "42831ec2217774244b7221b784d0d49c"
							  "e3aa212f2c02a4e035c17e2329aca12e"
							  "21d514b25466931c7d8f6a5aac84aa05"
							  "1ba30b396a0aac973d58e091473f5985",
                        "tag": "4d5c2af327cd64a62cf35abd2ba6fab4",
                    },
                },
            ],
        )

    def __init__(self, cipher: BlockCipher):
        self.n = cipher.blocksize()
        assert self.n == 16
        self.H = cipher.encrypt((0).to_bytes(self.n))
        self.ghash = GHASH(self.H)
        self.gctr = GCTR(cipher)

    def encrypt(self, iv: bytes, ad: bytes, pt: bytes):
        n = self.n
        J0 = (0).to_bytes(n)
        if len(iv) == 12:
            J0 = iv + (1).to_bytes(4, "big")
        else:
            s = 16 * ceil(len(iv) / 16) - len(iv)
            J0 = self.ghash.hash(
                iv + (0).to_bytes(s) + (len(iv) * 8).to_bytes(16, "big"))
        C = self.gctr.encrypt(inc32(J0), pt)
        u = ceil(len(C) / 16) * 16 - len(C)
        v = ceil(len(ad) / 16) * 16 - len(ad)
        S = self.ghash.hash(
            ad
            + (0).to_bytes(v)
            + C
            + (0).to_bytes(u)
            + ((len(ad) * 8).to_bytes(8, "big") + (len(C) * 8).to_bytes(8, "big"))
        )
        
        T = self.gctr.encrypt(J0, S)
        return (C, T)
