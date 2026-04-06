from typing import Protocol
from impl.specs import Field, SchemeSpec, Scheme

def make_common_spec_bc(name: str, n: int, fixed_vectors):
    return SchemeSpec(name, [ Field("pt", n, n), Field("key", n, n) ], ["ct"], fixed_vectors)

class BlockCipher(Scheme):
    @classmethod
    def blocksize(cls) -> int:
        ...
    
    def __init__(self, key:bytes):
        ...
    
    def encrypt(self, pt:bytes) -> bytes:
        ...
    def decrypt(self, pt:bytes) -> bytes:
        ...
        
    @staticmethod
    def encrypt_with_key(key:bytes, pt:bytes) -> bytes:
        ...
        
    @staticmethod
    def decrypt_with_key(key:bytes, ct:bytes) -> bytes:
        ...
