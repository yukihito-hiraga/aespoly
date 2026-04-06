from typing import Protocol

class Hash(Protocol):
    @property
    def hashsize(self) -> int:
        ...
    
    def __init__(self, key:bytes):
        ...
    
    def hash(self, pt:bytes) -> bytes:
        ...
        
    @staticmethod
    def hash_with_key(key:bytes, pt:bytes) -> bytes:
        ...
        
class TweakableHash(Protocol):
    @property
    def hashsize(self) -> int:
        ...
    
    def __init__(self, key:bytes):
        ...
    
    def hash(self, tweak:bytes, pt:bytes) -> bytes:
        ...
        
    @staticmethod
    def hash_with_key(key:bytes, tweak:bytes, pt:bytes) -> bytes:
        ...