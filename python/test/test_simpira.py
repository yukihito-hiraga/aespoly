import pytest

from random import randbytes
from impl.blockcipher.simpira import Simpira256, Simpira256EM

def test_mouha():
    simpira = Simpira256()
    x = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
    y = simpira.encrypt(x)
    assert y == bytes.fromhex("6b95ca7d8cda46cf97ab4430a8ef27c631b464a6ed106a553e30a83ba08c14c2")
 
def test_mouha_em_1():
    key = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
    simpira = Simpira256EM(key)
    x = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
    y = simpira.encrypt(x)
    assert y == bytes.fromhex("6b95ca7d8cda46cf97ab4430a8ef27c631b464a6ed106a553e30a83ba08c14c2")
    
def test_mouha_em_2():
    key = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000001")
    simpira = Simpira256EM(key)
    x = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000001")
    y = simpira.encrypt(x)
    assert y == bytes.fromhex("6b95ca7d8cda46cf97ab4430a8ef27c631b464a6ed106a553e30a83ba08c14c3")
    
def test_mouha_inv():
    simpira = Simpira256()
    y = bytes.fromhex("6b95ca7d8cda46cf97ab4430a8ef27c631b464a6ed106a553e30a83ba08c14c2")
    x = simpira.decrypt(y)
    assert x == bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
    
def test_mouha_em_1_inv():
    key = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
    simpira = Simpira256EM(key)
    y = bytes.fromhex("6b95ca7d8cda46cf97ab4430a8ef27c631b464a6ed106a553e30a83ba08c14c2")
    x = simpira.decrypt(y)
    assert x == bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000000")
    
def test_mouha_em_2_inv():
    key = bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000001")
    simpira = Simpira256EM(key)
    y = bytes.fromhex("6b95ca7d8cda46cf97ab4430a8ef27c631b464a6ed106a553e30a83ba08c14c3")
    x = simpira.decrypt(y)
    assert x == bytes.fromhex("0000000000000000000000000000000000000000000000000000000000000001")

def test_random_inv():
    key = randbytes(32)
    pt = randbytes(32)
    simpira = Simpira256EM(key)
    ct = simpira.encrypt(pt)
    ppt = simpira.decrypt(ct)
    assert ppt == pt