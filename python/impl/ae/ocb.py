from impl.xex import XEX, XE
from impl.common.gf import GF
from math import ceil
from Crypto.Util.strxor import strxor
from impl.common.utils import sumxor, pad, ozp
from impl.blockcipher.protocol import BlockCipher
from impl.specs import SchemeSpec, Field

class OCB:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
            f"ocb_{E.spec().name}",
            [
                Field("nonce", 0, 15),
                Field("pt", 0, None),
                Field("ad", 0, None),
                Field("key", 16, 16),
            ],
            ["ct"],
            [
                {
                    "scheme" : f"ocb_{E.spec().name}",
                    "inputs": {
                        "key": "000102030405060708090A0B0C0D0E0F",
                        "nonce": "BBAA99887766554433221100",
                        "ad": "",
                        "pt": "",
                    },
                    "expected": {
                        "ct": "785407BFFFC8AD9EDCC5520AC9111EE6",
                    },
                },
                {
                    "scheme" : f"ocb_{E.spec().name}",
                    "inputs": {
                        "key": "000102030405060708090A0B0C0D0E0F",
                        "nonce": "BBAA99887766554433221101",
                        "ad": "0001020304050607",
                        "pt": "0001020304050607",
                    },
                    "expected": {
                        "ct": "6820B3657B6F615A5725BDA0D3B4EB3A257C9AF1F8F03009",
                    },
                },
                {
                    "scheme" : f"ocb_{E.spec().name}",
                    "inputs": {
                        "key": "000102030405060708090A0B0C0D0E0F",
                        "nonce": "BBAA99887766554433221102",
                        "ad": "0001020304050607",
                        "pt": "",
                    },
                    "expected": {
                        "ct": "81017F8203F081277152FADE694A0A00",
                    },
                },
            ],
        )
    
    def __init__(self, cipher:BlockCipher):
        self.blocksize = cipher.blocksize()
        assert self.blocksize == 16
        self.cipher = cipher
        self.Lstar = cipher.encrypt((0).to_bytes(self.blocksize))
        field = self.field = GF((1 << 128) + 0x87)
        two = field.from_int(2)
        self.Ldollar = (field.from_bytes(self.Lstar, 'big') * two).to_bytes('big')
        self.L0 = L0 = (field.from_bytes(self.Ldollar, "big") * two).to_bytes('big')
        self.xex = XEX(cipher, 0x87, L0)
        self.xe = XE(cipher, 0x87, L0)

    def hash(self, ad: bytes) -> bytes:
        ct, offset = self.xe.encrypt_graycode(ad)
        s = sumxor(ct, self.blocksize)
        if len(ad) % self.blocksize > 0:
            offset_star = strxor(offset, self.Lstar)
            rem = len(ad) % self.blocksize
            ad_star = ad[-rem:]
            ci = strxor(ozp(ad_star, self.blocksize), offset_star)
            s = strxor(s, self.cipher.encrypt(ci))
        return s

    def encrypt(self, nonce: bytes, ad: bytes, pt: bytes):
        m = ceil(len(pt) / self.blocksize)
        TAGLEN = 16
        nonce = (
        	(((TAGLEN * 8 % 128) << (1 + (120-len(nonce)*8))) + 1).to_bytes(self.blocksize - len(nonce))
            + nonce
        )
        bottom = int.from_bytes(nonce[15:]) & 0b00111111
        ktop = self.cipher.encrypt((nonce[:15] + (nonce[15] & 0b11000000).to_bytes()))
        stretch = ktop + strxor(ktop[:8], ktop[1:9])
        offset0 = int(f'{int.from_bytes(stretch, 'big'):0{(self.blocksize+8)*8}b}'[bottom:bottom+128], 2).to_bytes(self.blocksize)
        ct, offset_m = self.xex.encrypt_graycode(pt, offset0)
        checksum = sumxor(pt, self.blocksize)

        ct_star = b''
        tag = b''

        if len(pt) % self.blocksize > 0:
            offset_star = strxor(offset_m, self.Lstar)
            rem = len(pt) % self.blocksize
            padding = self.cipher.encrypt(offset_star)
            pt_star = pt[-rem:]
            ct_star = strxor(pt_star, padding[:rem])
            checksum_star = strxor(checksum, ozp(pt_star, self.blocksize))
            tag = strxor(
                self.cipher.encrypt(strxor(checksum_star, strxor(offset_star, self.Ldollar))),
                self.hash(ad),
            )
        else:
            ct_star = b""
            tag = strxor(
                self.cipher.encrypt(strxor(checksum, strxor(offset_m, self.Ldollar))),
                self.hash(ad),
            )

        return ct + ct_star + tag[:TAGLEN]
