from impl.blockcipher.protocol import BlockCipher, make_common_spec_bc
from Crypto.Util.strxor import strxor
from impl.common.gf import GF
from functools import reduce
from impl.specs import SchemeSpec, Field

class Rijndael256(BlockCipher):
    def rotword(self, w: bytes, k=1) -> bytes:
        return b''.join([w[(i+k) % len(w)].to_bytes() for i in range(len(w))])

    def keyex(self, key: bytes):
        self.w[0:32] = key
        for i in range(8, 120):
            temp = self.w[4*(i-1):4*(i)]
            if self.verbose:
                print(i, "temp", temp.hex(), end="")
            if i % 8 == 0:
                temp = strxor(self.subbytes(
                    self.rotword(temp)), self.rcon[i // 8 - 1])
            elif i % 8 == 4:
                temp = self.subbytes(temp)
            self.w[4 * (i): 4 * (i + 1)] = strxor(self.w[4 *
                                                         (i-8): 4 * (i-8 + 1)], temp)
            if self.verbose:
                print(", w[i-Nk]", self.w[4 * (i-8) : 4 * (i-8 + 1)].hex(), end="")
                print(", w[i]", strxor(
                    self.w[4 * (i-8): 4 * (i-8 + 1)], temp).hex())

    def roundkey(self, i):
        return self.w[32*i:32*(i+1)]
    
    @classmethod
    def blocksize(cls) -> int:
        return 32
    
    @classmethod
    def spec(cls) -> SchemeSpec:
        return make_common_spec_bc(
            "rijndael256",
            32,
            [
                {
                    "scheme" : "rijndael256",
                    "inputs": {
                        "key": "0000000000000000000000000000000000000000000000000000000000000000",
                        "pt": "C6227E7740B7E53B5CB77865278EAB0726F62366D9AABAD908936123A1FC8AF3",
                    },
                    "expected": {
                        "ct": "9843E807319C32AD1EA3935EF56A2BA96E4BF19C30E47D88A2B97CBBF2E159E7",
                    },
                }
            ],
        )

    def __init__(self, key: bytes, verbose=False):
        self.verbose = verbose
        self.rcon = [
            b"\x01\x00\x00\x00",
        ]
        self.field = GF(sum([1 << x for x in [8, 4, 3, 1, 0]]))
        for i in range(1, 30):
            self.rcon.append((self.field.from_int(self.rcon[i-1][0]) * self.field.from_int(2)).to_bytes() + (0).to_bytes(3))
    
        self.w = bytearray([0 for _ in range(480)])
        self.sbox = [
            0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
            0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
            0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
            0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
            0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
            0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
            0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
            0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
            0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
            0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
            0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
            0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
            0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
            0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
            0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
            0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
        ]
        self.invsbox = [
            0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb, 
            0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 
            0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e, 
            0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25, 
            0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 
            0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84, 
            0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06, 
            0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 
            0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73, 
            0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e, 
            0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 
            0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4, 
            0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f, 
            0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 
            0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61, 
            0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
        ]
        
        self.keyex(key)
        self.M = [
            [
                self.field.from_int(2),
                self.field.from_int(3),
                self.field.from_int(1),
                self.field.from_int(1),
            ],
            [
                self.field.from_int(1),
                self.field.from_int(2),
                self.field.from_int(3),
                self.field.from_int(1),
            ],
            [
                self.field.from_int(1),
                self.field.from_int(1),
                self.field.from_int(2),
                self.field.from_int(3),
            ],
            [
                self.field.from_int(3),
                self.field.from_int(1),
                self.field.from_int(1),
                self.field.from_int(2),
            ],
        ]
        self.invM = [
            [
                self.field.from_int(0x0e),
                self.field.from_int(0x0b),
                self.field.from_int(0x0d),
                self.field.from_int(0x09),
            ],
            [
                self.field.from_int(0x09),
                self.field.from_int(0x0e),
                self.field.from_int(0x0b),
                self.field.from_int(0x0d),
            ],
            [
                self.field.from_int(0x0d),
                self.field.from_int(0x09),
                self.field.from_int(0x0e),
                self.field.from_int(0x0b),
            ],
            [
                self.field.from_int(0x0b),
                self.field.from_int(0x0d),
                self.field.from_int(0x09),
                self.field.from_int(0x0e),
            ],
        ]

    def subbytes(self, state: bytes):
        return b''.join([self.sbox[b].to_bytes() for b in state])

    def shiftrows(self, state: bytes):
        rs = [
            self.rotword(bytes([4*i+0 for i in range(8)]), 0),
            self.rotword(bytes([4*i+1 for i in range(8)]), 1),
            self.rotword(bytes([4*i+2 for i in range(8)]), 3),
            self.rotword(bytes([4*i+3 for i in range(8)]), 4),
        ]
        return b''.join([
            state[rs[i % 4][i//4]].to_bytes()
            for i in range(32)])

    def mixcolumns(self, state: bytes):
        res = bytearray([0 for _ in range(32)])
        for k in range(8):
            s = state[4*k:4*(k+1)]
            ss = [
                reduce(lambda s, x: s+x, [
                    self.M[i][j] * self.field.from_int(s[j])
                    for j in range(4)
                ]).value
                for i in range(4)
            ]
            res[4 * k: 4 * (k + 1)] = ss
        return res
    
    def invsubbytes(self, state: bytes):
        return b''.join([self.invsbox[b].to_bytes() for b in state])

    def invshiftrows(self, state: bytes):
        rs = [
            self.rotword(bytes([4*i+0 for i in range(8)]), 0),
            self.rotword(bytes([4*i+1 for i in range(8)]), -1),
            self.rotword(bytes([4*i+2 for i in range(8)]), -3),
            self.rotword(bytes([4*i+3 for i in range(8)]), -4),
        ]
        return b''.join([
            state[rs[i % 4][i//4]].to_bytes()
            for i in range(32)])

    def invmixcolumns(self, state: bytes):
        res = bytearray([0 for _ in range(32)])
        for k in range(8):
            s = state[4*k:4*(k+1)]
            ss = [
                reduce(lambda s, x: s+x, [
                    self.invM[i][j] * self.field.from_int(s[j])
                    for j in range(4)
                ]).value
                for i in range(4)
            ]
            res[4 * k: 4 * (k + 1)] = ss
        return res

    def print_state(self, state: bytes):
        for i in range(32):
            print("{:02x}".format(state[(i % 8)*4+(i//8)]), end="")
            if i % 8 == 7:
                print()
        print()

    def round(self, state: bytes, key: bytes, verbose=False) -> bytes:
        state = self.subbytes(state)
        state = self.shiftrows(state)
        state = self.mixcolumns(state)
        state = strxor(state, key)
        return state

    def lastround(self, state: bytes, key: bytes, verbose=False) -> bytes:
        state = self.subbytes(state)
        state = self.shiftrows(state)
        state = strxor(state, key)
        return state
    
    def invround(self, state: bytes, key: bytes, verbose=False) -> bytes:
        state = strxor(state, key)
        state = self.invmixcolumns(state)
        state = self.invshiftrows(state)
        state = self.invsubbytes(state)
        return state

    def invlastround(self, state: bytes, key: bytes, verbose=False) -> bytes:
        state = strxor(state, key)
        state = self.invshiftrows(state)
        state = self.invsubbytes(state)
        return state
    
    def encrypt(self, pt: bytes, verbose=False):
        state = pt
        state = strxor(state, self.roundkey(0))
        for i in range(1, 14):
            if verbose:
                print("round {}".format(i))
            state = self.round(state, self.roundkey(i), verbose=verbose)
        if verbose:
            print("round 14")
        state = self.lastround(state, self.roundkey(14), verbose=verbose)
        return state
    
    def decrypt(self, ct: bytes) -> bytes:
        state = ct
        state = self.invlastround(state, self.roundkey(14))
        for i in range(13, 0, -1):
            state = self.invround(state, self.roundkey(i))
        state = strxor(state, self.roundkey(0))
        return state
    
    @staticmethod
    def encrypt_with_key(key: bytes, pt: bytes) -> bytes:
        bc = Rijndael256(key)
        ct = bc.encrypt(pt)
        return ct
    
    @staticmethod
    def decrypt_with_key(key: bytes, ct: bytes) -> bytes:
        return Rijndael256(key).decrypt(ct)
