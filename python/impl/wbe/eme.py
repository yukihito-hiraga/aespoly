from impl.common.gf import GF
from impl.common.utils import partition
from Crypto.Util.strxor import strxor
from Crypto.Util.Padding import pad
from functools import reduce
from impl.blockcipher.protocol import BlockCipher
from impl.specs import SchemeSpec, Field

class EME:
    @classmethod
    def spec_of(cls, E: type[BlockCipher]) -> SchemeSpec:
        return SchemeSpec(
            f"eme_{E.spec().name}",
            [
                Field("pt", E.blocksize(), None, E.blocksize()),
                Field("tweak", E.blocksize(), E.blocksize()),
                Field("key", E.blocksize(), E.blocksize()),
            ],
            ["ct"],
            [],
        )
    
    def __init__(self, cipher:BlockCipher):
        self.cipher = cipher
        self.n = cipher.blocksize()
        self.field = GF((1 << (self.n*8)) + 0x87)

    def encrypt(self, T: bytes, P: bytes, verbose=False):
        T = pad(T, self.n)[:self.n]
        two = self.field.from_int(2)
        zeros = (0).to_bytes(self.n)
        Lprev = self.field.from_int(int.from_bytes(
            self.cipher.encrypt(zeros), "big"))
        L = Lprev

        if verbose:
            print(f"Ek(0) {self.cipher.encrypt(zeros).hex()}")
            print("Lprev", Lprev)
            print("L", L)

        LL = L
        PPPs = []
        SP_PPP1 = (0).to_bytes(self.n)

        for i, Pi in enumerate(partition(P, self.n)):
            PPi = strxor(Pi, LL.to_bytes("big"))
            PPPi = self.cipher.encrypt(PPi)
            if verbose:
                print("P{}".format(i+1), Pi.hex())
                print("PP{}".format(i+1), PPi.hex())
                print("PPP{}".format(i+1), PPPi.hex())
                print("L{}".format(i+1), LL)
                print()
            PPPs.append(PPPi)
            SP_PPP1 = strxor(PPPi, SP_PPP1)
            LL = LL * two

        if verbose:
            print("PPP1", PPPs[0].hex())

        MP = strxor(SP_PPP1, T)
        MC = self.cipher.encrypt(MP)
        M = self.field.from_bytes(strxor(MP, MC), "big")

        if verbose:
            print("MP", MP.hex())
            print("MC", MC.hex())
            print("M", M)

        MM = M * two
        CCCs = [strxor(MC, T)]
        SC = (0).to_bytes(self.n)

        for i, PPPi in enumerate(PPPs[1:]):
            CCCi = strxor(MM.to_bytes("big"), PPPi)
            CCCs.append(CCCi)
            if verbose:
                print("PPP{}".format(i+2), PPPi.hex())
                print("CCC{}".format(i+2), CCCi.hex())
                print("M{}".format(i+2), MM)
                print("SC{}".format(i+2), SC.hex())
                print()
            MM = MM * two
            SC = strxor(SC, CCCi)

        CCCs[0] = strxor(CCCs[0], SC)

        if verbose:
            print("SC", SC.hex())
            print("CCC1", CCCs[0].hex())

        LL = L
        Cs = []
        CCs = []
        for CCCi in CCCs:
            CCi = self.cipher.encrypt(CCCi)
            Ci = strxor(CCi, LL.to_bytes("big"))
            LL = LL * two
            Cs.append(Ci)
            CCs.append(CCi)
        if verbose:
            print("SC", SC.hex())
            print("CC1", CCs[0].hex())
        return reduce(lambda s, x: s + x, Cs)
