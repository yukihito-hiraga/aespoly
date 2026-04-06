from impl.blockcipher.protocol import BlockCipher
from impl.common.utils import partition, sumxor
from impl.xex import XE, EX
from impl.xctr import XCTR
from Crypto.Util.strxor import strxor
from functools import reduce
from impl.hash.polyval import Polyval, DBPolyval
from impl.hash.protocol import TweakableHash
from math import floor, ceil
from impl.specs import SchemeSpec, Field

class PHASH:
    def __init__(self, cipher, delta):
        self.cipher = cipher
        self.blocksize = cipher.blocksize()
        self.xe = XE(cipher, 0x87, delta)

    def hash(self, pt:bytes):
        ct = self.xe.encrypt(pt)
        return reduce(lambda s,x:strxor(s, x), partition(ct, self.blocksize), (0).to_bytes(self.blocksize))

def copyc(N:bytes, c:int) -> bytes:
    return reduce(lambda x,s: s + x, [ N for _ in range(c) ], b'')

def xorn(D:bytes, n:int) -> bytes:
    return reduce(lambda x,s: strxor(x,s), partition(D, n), (0).to_bytes(n))
            

class AESpolyW:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
			f"aespolyW_{E.spec().name}",
			[
				Field("key", E.blocksize(), E.blocksize()),
				Field("h", E.blocksize(), E.blocksize()),
				Field("tweak", 0, None),
				Field("pt", E.blocksize(), None),
			],
			["ct"],
			[]
		)
    
    def __init__(self, cipher:BlockCipher, hash:TweakableHash):
        self.name = "aespolyW"
        self.blocksize = cipher.blocksize()
        self.cipher = cipher
        self.H = hash
        self.delta0 = cipher.encrypt((0).to_bytes(self.blocksize))
        self.delta1 = cipher.encrypt(( (1 << (8*self.blocksize)) -1 ).to_bytes(self.blocksize))
        self.phash = PHASH(cipher, self.delta0)
        self.xe = XE(cipher, 0x87, self.delta1)
        self.ex = EX(cipher, 0x87, self.delta1)
        self.xctr = XCTR(cipher)


    def devide(self, pt:bytes, tw:bytes):
        mn = ceil(len(pt) / self.blocksize)
        tn = ceil(len(tw) / self.blocksize)
        mr = floor(0.5 * (mn-1))
        tr = floor(0.5 * tn)
        M0 = pt[:self.blocksize]
        Mr = pt[-mr*self.blocksize:] if mr > 0 else b''
        Ml = pt[self.blocksize:-mr*self.blocksize] if mr > 0 else pt[self.blocksize:]
        Tr = tw[-tr*self.blocksize:] if tr > 0 else b''
        Tl = tw[:-tr*self.blocksize] if tr > 0 else tw
        return ((M0, Ml, Mr), (Tl, Tr))

    def encrypt(self, tweak, pt, verbose=False):
        mn = ceil(len(pt) / self.blocksize)
        mr = floor(0.5 * (mn-1))
        ((M0, Ml, Mr), (Tl, Tr)) = self.devide(pt, tweak)

        S = self.phash.hash(Tr)
        Ul = self.H.hash(Tl, Ml)
        
        Y1 = self.xe.encrypt(Mr)
        Ur = xorn(Y1+S,self.blocksize)
        
        X0 = strxor(strxor(Ul, Ur), M0)
        N = self.cipher.encrypt(X0)
        
        Cl = self.xctr.encrypt(N, Ml)
        X3 = strxor(Y1, copyc(N, mr))

        Y0 = self.cipher.encrypt(N)
        Vl = self.H.hash(Tl, Cl)

        Vr = xorn(X3+S, self.blocksize)
        Cr = self.ex.encrypt(X3)
        
        C0 = strxor(strxor(Vl, Vr), Y0)
        C = C0 + Cl + Cr
        
        return C
    
    def decrypt(self, tweak, ct, verbose=False):
        cn = ceil(len(ct) / self.blocksize)
        cr = floor(0.5 * (cn-1))
        ((C0, Cl, Cr), (Tl, Tr)) = self.devide(ct, tweak)
        if verbose:
            print("C0:", C0.hex())
            print("Cl:", Cl.hex())
            print("Cr:", Cr.hex())
            print("Tl:", Tl.hex())
            print("Tr:", Tr.hex())

        S = self.phash.hash(Tr)
        Vl = self.H.hash(Tl, Cl)
        
        X3 = self.xe.encrypt(Tr)
        Vr = self.H.hash(Tl, Cl)
        
        Y0 = strxor(strxor(Vl, Vr), C0)
        N = self.cipher.decrypt(Y0)
        
        Ml = self.xctr.encrypt(N, Cl)
        Y1 = strxor(X3, copyc(N, cr))
        
        Mr = self.ex.encrypt(Y1)
        X0 = self.cipher.decrypt(N)
        
        Ul = self.H.hash(Tl, Ml)
        Ur = xorn(Y1+S, self.blocksize)
        
        M0 = strxor(strxor(Ul, Ur), X0)
        M = M0 + Ml + Mr
        
        return M
