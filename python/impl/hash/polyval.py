from impl.common.gf import GF
from impl.hash.protocol import Hash

class Polyval(Hash):
    @property
    def hashsize(self) -> int:
        return 16
    
    def __init__(self, key):
        self.field = GF(sum(1 << x for x in [128, 127, 126, 121, 0]))
        self.inv = self.field.from_int(sum(1 << x for x in [127, 124, 121, 114, 0]))
        self.h = key
        pass

    def hash(self, pt, state: GF.Element | None =None, verbose=False) -> bytes:
        block_size = self.field.n // 8
        hgen = self.field.from_bytes(self.h, 'little')
        hpoly = hgen * self.inv
        if verbose:
            print(f"hgen : {hgen.to_bytes('little').hex()}")
            print(f"hpoly : {hpoly.to_bytes('little').hex()}")
        X = self.field.from_int(0)
        if state != None:
            X = state
        for i in range(0, len(pt), block_size):
            B = self.field.from_bytes(pt[i:i+block_size], 'little')
            X = X + B
            X = X * hpoly
        return X.to_bytes('little')

    @staticmethod
    def hash_with_key(key: bytes, pt: bytes, state: GF.Element | None =None, verbose=False) -> bytes:
        polyval = Polyval(key)
        return polyval.hash(pt, state=state, verbose=verbose)

class DBPolyval(Hash):
    @property
    def hashsize(self) -> int:
        return 32
    
    def __init__(self, key):
        self.hashes = [ Polyval(key[:16]), Polyval(key[16:]) ]
    
    def hash(self, pt: bytes, state: GF.Element | None = None, verbose=False) -> bytes:
        return self.hashes[0].hash(pt, state=state, verbose=verbose) + self.hashes[1].hash(pt, state=state, verbose=verbose)
    
    @staticmethod
    def hash_with_key(key: bytes, pt: bytes, state: GF.Element | None =None, verbose=False) -> bytes:
        dbpolyval = DBPolyval(key)
        return dbpolyval.hash(pt, state=state, verbose=verbose)