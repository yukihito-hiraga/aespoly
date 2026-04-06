from impl.blockcipher.aes import AES128
from impl.blockcipher.protocol import BlockCipher, make_common_spec_bc
from Crypto.Util.strxor import strxor
from impl.specs import SchemeSpec
from impl.common.utils import partition

def bitflip(b:bytes):
    l = len(b)
    s = f'{int.from_bytes(b, 'little'):0{8*l}b}'
    rs = s[::-1]
    return int(rs, 2).to_bytes(l, 'little')

class Simpira256:
    def __init__(self):
        self.block_size = 32
        self.aes = AES128((0).to_bytes(16))

    def round(self, c:int, y):
        rr = [
                (c ^ 0x02).to_bytes(4, 'little'),
                (c ^ 0x12).to_bytes(4, 'little'),
                (c ^ 0x22).to_bytes(4, 'little'),
                (c ^ 0x32).to_bytes(4, 'little'),
            ]
        # rr.reverse()
        C = b''.join(
            rr
        )
        t1 = self.aes.round(y[0][::], C[::])[::]
        t2 = self.aes.round(t1[::], y[1][::])[::]

        return t2

    def encrypt(self, pt: bytes):
        assert len(pt) == 32
        x = partition(pt, 16)
        for c in range(1, 16):
            if c % 2 == 1:
                x[1] = self.round(c, [x[0], x[1]])
            else:
                x[0] = self.round(c, [x[1], x[0]])
        return b''.join(x)
    
    def decrypt(self, ct: bytes):
        assert len(ct) == 32
        x = partition(ct, 16)
        for c in range(15, 0, -1):
            if c % 2 == 1:
                x[1] = self.round(c, [x[0], x[1]])
            else:
                x[0] = self.round(c, [x[1], x[0]])
        return b''.join(x)

class Simpira256EM(BlockCipher):
    @classmethod
    def blocksize(cls) -> int:
        return 32
    
    @classmethod
    def spec(cls) -> SchemeSpec:
        return make_common_spec_bc(
            "simpira256",
            32,
            [
                {
                    "scheme" : "simpira256",
                    "inputs": {
                        "key": "0000000000000000000000000000000000000000000000000000000000000000",
                        "pt": "0000000000000000000000000000000000000000000000000000000000000000",
                    },
                    "expected": {
                        "ct": "9843E807319C32AD1EA3935EF56A2BA96E4BF19C30E47D88A2B97CBBF2E159E7",
                    },
                }
            ],
        )
    
    def __init__(self, key: bytes):
        assert len(key) == 32
        self.key = key
        self.block_size = 32
        self.simpira = Simpira256()

    def encrypt(self, pt: bytes):
        assert len(pt) == 32
        temp = strxor(pt, self.key)
        y = self.simpira.encrypt(temp)
        return strxor(y, self.key)
    
    def decrypt(self, pt: bytes) -> bytes:
        assert len(pt) == 32
        temp = strxor(pt, self.key)
        y = self.simpira.decrypt(temp)
        return strxor(y, self.key)
    
    @staticmethod
    def encrypt_with_key(key: bytes, pt: bytes) -> bytes:
        return Simpira256EM(key).encrypt(pt)
    
    @staticmethod
    def decrypt_with_key(key: bytes, ct: bytes) -> bytes:
        return Simpira256EM(key).decrypt(ct)