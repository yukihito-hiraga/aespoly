from Crypto.Util.strxor import strxor
from functools import reduce

def partition(P:bytes, n : int):
    Ps = [
        P[i*n:(i+1)*n]
        for i in range(len(P)//n)
    ]
    return Ps

def sumxor(P:bytes, n:int):
    return reduce(lambda s,x:strxor(s, x), partition(P, n), (0).to_bytes(n))

def pad(P:bytes, n:int):
    rem = len(P) % n
    rest = (n - rem) % n
    res = P + (0).to_bytes(rest)
    return  res

def ozp(P:bytes, n:int):
    if len(P) % n == 0 and len(P) > 0:
        return P
    return pad(P + b'\x80', n)


def printb(name, b):
    print(f"{name}\t {b.hex()}:{int.from_bytes(b):0128b}")


def printX(name, X):
    print(f"{name}\t {X.to_bytes("big").hex()}:{X.value:0128b}")
