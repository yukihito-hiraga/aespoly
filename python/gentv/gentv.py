from pathlib import Path
from subprocess import run
from random import randbytes, randint
import json

from impl.specs import SchemeSpec, Field

from impl.blockcipher.aes import AES128
from impl.ae.gcm import GCM
from impl.mac.gmac import GMAC
from impl.ae.ocb import OCB
from impl.mac.pmac import PMAC
from impl.blockcipher.rijndael256 import Rijndael256
from impl.blockcipher.simpira import Simpira256EM
from impl.wbe.hctr2 import HCTR2
from impl.wbe.eme import EME
from impl.wbe.aespolyW import AESpolyW
from impl.mac.aespolyM import AESpolyM

import impl.wbe.hctr2 as hctr2

def gentv(path: Path, spec: SchemeSpec, n=10, run=None):
    def randlength(field : Field):
        a = field.min_length if field.min_length != None else 0
        b = field.max_length if field.max_length != None else 80
        return randint(a, b)//field.unit * field.unit
    
    with open(path/f"{spec.name}.jsonl", "w") as f:
        if run is not None:
            for _ in range(n):
                inputs = {
                    field.name : randbytes(randlength(field))
                    for field in spec.inputs
                }
                output = run(**inputs)
                obj = {
                    "scheme" : spec.name,
                    "inputs" : {
                        k : v.hex()
                        for k, v in inputs.items()
                    },
                    "expected" : {
                        k : v.hex()
                        for k, v in output.items()
                    }
                }
                f.write(f"{json.dumps(obj)}\n")
        for ftv in spec.fixed_vectors:
            f.write(f"{json.dumps(ftv)}\n")

def main():
    dir = Path(__file__).parents[2]/"testvectors"

    run(f"mkdir -p {dir}", shell=True, capture_output=True)
    [
        gentv(dir, spec, run=run, n=1)
        for spec, run in [
            # (AES128.spec(), lambda pt, key: {"ct" : AES128.encrypt_with_key(key, pt)}),
            (GCM.spec_of(AES128), lambda key, iv, ad, pt: [
                {
                    "ct" : res[0],
                    "tag" : res[1],
                }
                for res in [GCM(AES128(key)).encrypt(iv, ad, pt)]
            ][0]),
            (GMAC.spec_of(AES128), lambda key, iv, ad: [
                {
                    "tag" : res,
                }
                for res in [GMAC(AES128(key)).hash(iv, ad)]
            ][0]),
            (OCB.spec_of(AES128), lambda key, nonce, ad, pt: [
                {
                    "ct" : res,
                }
                for res in [OCB(AES128(key)).encrypt(nonce, ad, pt)]
            ][0]),
            (PMAC.spec_of(AES128), lambda key, pt: [
                {
                    "tag" : res,
                }
                for res in [PMAC(AES128(key)).hash(pt)]
            ][0]),
            # (Rijndael256.spec(), lambda pt, key: {"ct" : Rijndael256.encrypt_with_key(key, pt)}),
            # (Simpira256EM.spec(), lambda pt, key: {"ct" : Simpira256EM.encrypt_with_key(key, pt)}),
            (HCTR2.spec_of(AES128), lambda key, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [HCTR2(AES128(key)).encrypt(tweak, pt)]
            ][0]),
            (HCTR2.spec_of(Rijndael256), lambda key, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [HCTR2(Rijndael256(key)).encrypt(tweak, pt)]
            ][0]),
            (HCTR2.spec_of(Simpira256EM), lambda key, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [HCTR2(Simpira256EM(key)).encrypt(tweak, pt)]
            ][0]),
            (EME.spec_of(AES128), lambda key, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [EME(AES128(key)).encrypt(tweak, pt)]
            ][0]),
            (EME.spec_of(Rijndael256), lambda key, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [EME(Rijndael256(key)).encrypt(tweak, pt)]
            ][0]),
            (EME.spec_of(Simpira256EM), lambda key, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [EME(Simpira256EM(key)).encrypt(tweak, pt)]
            ][0]),
            (AESpolyW.spec_of(AES128), lambda key, h, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [AESpolyW(AES128(key), hctr2.Hash(h)).encrypt(tweak, pt)]
            ][0]),
            (AESpolyW.spec_of(Rijndael256), lambda key, h, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [AESpolyW(Rijndael256(key), hctr2.Hash(h)).encrypt(tweak, pt)]
            ][0]),
            (AESpolyW.spec_of(Simpira256EM), lambda key, h, tweak, pt: [
                {
                    "ct" : res,
                }
                for res in [AESpolyW(Simpira256EM(key), hctr2.Hash(h)).encrypt(tweak, pt)]
            ][0]),
            (AESpolyM.spec_of(AES128), lambda key1, key2, h, pt: [
                {
                    "tag" : res,
                }
                for res in [AESpolyM(AES128(key1), AES128(key2), hctr2.Hash(h)).hash(pt)]
            ][0]),
        ]
    ]


if __name__ == "__main__":
    main()
