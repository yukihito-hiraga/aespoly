from impl.common.gf import GF
from impl.common.utils import partition
from impl.blockcipher.protocol import BlockCipher
from Crypto.Util.strxor import strxor

def ntz(x:int):
	if x == 0:
		return 0
	res = 0
	while (x&1) == 0:
		x = x >> 1
		res = res + 1
	return res

class XEXCore:
    def __init__(self, cipher, poly, L):
        self.cipher = cipher
        self.blocksize = cipher.blocksize()
        self.poly = poly.to_bytes(self.blocksize, 'big')
        self.field = GF((1 << (self.blocksize * 8)) + poly)
        self.L = self.field.from_bytes(L, "little")
        self.two = self.field.from_int(2)

    def pow(self, i):
        res = self.L.to_bytes()
        for k in range(i):
            cond = (res[self.blocksize-1] & 0x80) != 0
            # res = (int.from_bytes(res, 'big') << 1).to_bytes(self.blocksize, 'big')
            res = int(
                f"{int.from_bytes(res, 'big'):0{self.blocksize*8}b}"[1:] + "0", 2
            ).to_bytes(self.blocksize, "big")
            if cond:
                res = strxor(res, self.poly)
        return res

    def gen_mask(self, i):
        return self.pow(i + 1)

    def seq_offset(self, offset, i, verbose=False):
        if verbose:
            for k in range(i+1):
                print(f"ntz({k})={ntz(k)}")
                print(f"L[ntz({k})]={self.pow(ntz(k)).hex()}")
        return strxor(offset, self.pow(ntz(i)))


class XEX:
    def __init__(self, cipher, poly, L):
        self.core = XEXCore(cipher, poly, L)
        self.blocksize = self.core.blocksize

    def encrypt(self, pt: bytes):
        ct = b""
        for i, pi in enumerate(partition(pt, self.blocksize)):
            temp = strxor(pi, self.core.gen_mask(i))
            ci = self.core.cipher.encrypt(temp)
            ci = strxor(ci, self.core.gen_mask(i))
            ct = ct + ci
        return ct

    def encrypt_graycode(self, pt: bytes, offset: bytes | None = None, verbose=False):
        ct = b""
        if offset == None:
            offset = (0).to_bytes(self.blocksize)
        for i, pi in enumerate(partition(pt, self.blocksize)):
            offset = self.core.seq_offset(offset, i+1, verbose)
            temp = strxor(pi, offset)
            ci = self.core.cipher.encrypt(temp)
            if verbose:
                print("offset", offset.hex())
                print("pi", pi.hex())
                print("pi^offset", temp.hex())
                print("ci", ci.hex())
            ci = strxor(ci, offset)
            ct = ct + ci
        return ct, offset


class XE:
    def __init__(self, cipher, poly, L):
        self.core = XEXCore(cipher, poly, L)
        self.blocksize = self.core.blocksize

    def encrypt(self, pt: bytes):
        ct = b""
        for i, pi in enumerate(partition(pt, self.blocksize)):
            temp = strxor(pi, self.core.gen_mask(i))
            ci = self.core.cipher.encrypt(temp)
            ct = ct + ci
        return ct

    def encrypt_graycode(self, pt: bytes, offset: bytes | None = None):
        ct = b""
        if offset == None:
            offset = (0).to_bytes(self.blocksize)
        for i, pi in enumerate(partition(pt, self.blocksize)):
            offset = self.core.seq_offset(offset, i+1)
            temp = strxor(pi, offset)
            ci = self.core.cipher.encrypt(temp)
            ct = ct + ci
        return ct, offset


class EX:
    def __init__(self, cipher, poly, L):
        self.core = XEXCore(cipher, poly, L)
        self.blocksize = self.core.blocksize

    def encrypt(self, pt: bytes):
        ct = b""
        for i, pi in enumerate(partition(pt, self.blocksize)):
            temp = self.core.cipher.encrypt(pi)
            ci = strxor(temp, self.core.gen_mask(i))
            ct = ct + ci
        return ct

    def encrypt_graycode(self, pt: bytes, offset: bytes | None = None):
        ct = b""
        if offset == None:
            offset = (0).to_bytes(self.blocksize)
        for i, pi in enumerate(partition(pt, self.blocksize)):
            offset = self.core.seq_offset(offset, i+1)
            temp = strxor(pi, offset)
            temp = self.core.cipher.encrypt(pi)
            ci = strxor(temp, offset)
            ct = ct + ci
        return ct, offset
