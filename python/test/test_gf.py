import pytest

from impl.common.gf import GF

def test_simple1():
    F = GF(sum(1 << x for x in [128, 7, 2, 1, 0]))
    REF_H = F.from_int(0x74D42C539A5F3211DC3451F72BD29766)
    REF_C = F.from_int(0x1E7F4D8E9D4314CF49C56D06735B11C0)
    U1 = REF_H * REF_C
    assert U1.value == 0xED7BCACA160DA13411460E8962E3747A
    REF_U1 = F.from_int(0x5E2EC746917062882C85B0685353DEB7)
    REF_LEN = F.from_int(0x1000000000000000000000000000000)
    U2 = (U1 + REF_LEN) * REF_H
    assert U2.value == 0xA11F0D6DA75EA2C33BC4496B58DD31CF
    
def test_simple2():
    F = GF(sum(1 << x for x in [128, 127, 126, 121, 0]))
    H = F.from_int(0x66E94BD4EF8A2C3B884CFA59CA342B2E)
    C = F.from_int(0x0388DACE60B6A392F328C2B971B2FE78)
    LEN = F.from_bytes( (128).to_bytes(8, 'little') + (0).to_bytes(8), 'little' )
    x = F.from_int(2)
    HH = H * x
    assert HH.value == 0xCDD297A9DF1458771099F4B39468565C
    xinv = F.from_int(sum(1 << x for x in [127, 124, 121, 114, 0]))
    TEMP1 = C * HH * xinv
    assert TEMP1.value == 0x5E2EC746917062882C85B0685353DEB7
    TEMP2 = (TEMP1 + LEN) * HH * xinv
    assert TEMP2.value == 0xF38CBB1AD69223DCC3457AE5B6B0F885