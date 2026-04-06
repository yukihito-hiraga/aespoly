from typing import Self, Literal

class GF:
    class Element:
        def __init__(self, field, value: int = 0):
            self.field = field
            self.value = value

        def __xor__(self, y) -> Self:
            assert self.field.n == y.field.n and self.field.n == y.field.n
            return self.field.from_int(self.field._add(self.value, y.value))

        def __add__(self, y) -> Self:
            return self.__xor__(y)

        def __mul__(self, y) -> Self:
            assert self.field == y.field
            return self.field.from_int(self.field._mul(self.value, y.value))
        
        def to_bytes(self, byteorder="little"):
            return (self.value & ((1 << self.field.n)-1)).to_bytes(self.field.n // 8, byteorder=byteorder)
        
        def __str__(self):
            return self.to_bytes("big").hex()
    
    def __init__(self, poly: int | bytes, byteorder:Literal['little', 'big']='little'):
        if isinstance(poly, int):
            self.n = 0
            while ( (poly >> (self.n+1)) > 0):
                self.n += 1
            self.xn = poly & ((1 << self.n)-1)
        else:
            self.n = len(poly)
            self.xn = int.from_bytes(poly, byteorder) & ((1 << self.n)-1)
    
    def _add(self, x, y):
        return x^y

    def _mul(self, x, y):
        res = 0
        c = x
        mask = (1 << self.n)-1
        while y > 0:
            res = res ^ ((y&1)*c)
            c = (mask & (c << 1)) ^ ((c >> (self.n-1)) * self.xn)
            y = y >> 1
        return res

    def from_int(self, value: int):
        return self.Element(self, value)
    
    def from_bytes(self, value:bytes, byteorder:Literal['little', 'big']):
        return self.from_int(int.from_bytes(value, byteorder=byteorder))

def bitref(x:int, n:int=0):
    res = 0
    while(x > 0 or n > 0):
        res = (res << 1) ^ (x & 1)
        x = x >> 1
        n = n - 1
    return res