from impl.common.gf import GF
from impl.hash.protocol import Hash

class GHASH(Hash):
    @property
    def hashsize(self) -> int:
        return 16
    
    def __init__(self, key):
        self.field = GF(sum(1 << x for x in [128, 127, 126, 121, 0]))
        self.inv = self.field.from_int(sum(1 << x for x in [127, 124, 121, 114, 0]))
        self.key = key

    def hash(self, pt) -> bytes:
        block_size = self.field.n // 8
        assert len(pt) % block_size == 0
        two = self.field.from_int(2)
        hgen = self.field.from_bytes(self.key, 'big') * two
        hpoly = hgen * self.inv
        X = self.field.from_int(0)
        for i in range(0, len(pt), block_size):
            B = self.field.from_bytes(pt[i:i+block_size], 'big')
            X = X + B
            X = X * hpoly
        return X.to_bytes('big')
    
    @staticmethod
    def hash_with_key(key: bytes, pt: bytes) -> bytes:
        return GHASH(key).hash(pt)