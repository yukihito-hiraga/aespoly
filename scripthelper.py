from pathlib import Path

def load_csv(name):
	result_dir = Path(__file__).parent / "result" / "result-2048-80"
	res = []
	with open(result_dir / name, "r") as f:
		for line in f:
			res.append([ round(float(s), 3) for s in line.strip().split(",")])
	return res

def ext_value(name):
	result_dir = Path(__file__).parent / "result" / "result-2048-80"
	if (result_dir / name).exists():
		return load_csv(name)[-1][-1]
	return None

def table4(x=8):
	print(f"Result for Table 4")
	CA = ext_value("hctr2-aes128.xctr.csv")
	CP = ext_value(f"hctr2-aes128.polyvalx{x}.csv")
	CAP = ext_value(f"hctr2-aes128.xctr‖polyvalx{x}.csv")
	if None in [CA, CP, CAP]:
		print("Not supported this experiment on this machine.")
		return
	rAP = round(CAP / (CA + CP), 3)
	
	print(f"CA: {CA}")
	print(f"CP: {CP}")
	print(f"CAP: {CAP}")
	print(f"rAP: {rAP}")

def table5(x=8):
	print(f"Result for Table 5")
	OCB = ext_value(f"ocb.ocbx{x}_offline.csv")
	GCM = ext_value(f"aesgcm.aesgcmx{x}.csv")
	if None in [OCB, GCM]:
		print("Not supported this experiment on this machine.")
		return

	ratio = round(OCB/GCM, 3) * 100
	print(f"AES-OCB: {OCB}")
	print(f"AES-GCM: {GCM}")
	print(f"AES-OCB/AES-GCM: {ratio}%")

def table6():
	print(f"Result for Table 6")
	CA = ext_value("hctr2-avx512.xctr.csv")
	CP = ext_value("hctr2-avx512.polyvalx4.csv")
	CAP = ext_value("hctr2-avx512.xctr‖polyvalx4.csv")
	if None in [CA, CP, CAP]:
		print("Not supported this experiment on this machine.")
		return
	rAP = round(CAP / (CA + CP), 3)
	
	print(f"CA: {CA}")
	print(f"CP: {CP}")
	print(f"CAP: {CAP}")
	print(f"rAP: {rAP}")

def table7(x=8):
	print(f"Result for Table 7")
	HCTR2 = ext_value(f"hctr2-aes128.hctr2x{x}.csv")
	EME = ext_value(f"eme-aes128.emex{x}.csv")
	AESpolyW = ext_value(f"aespoly-aes128.aespolyx{x}.csv")
	if None in [HCTR2, EME, AESpolyW]:
		print("Not supported this experiment on this machine.")
		return
	ratio_hctr2 = round(AESpolyW/HCTR2, 3) * 100
	ratio_eme = round(AESpolyW/EME, 3) * 100

	print(f"HCTR2: {HCTR2}")
	print(f"EME: {EME}")
	print(f"AESpolyW: {AESpolyW}")
	print(f"AESpolyW/HCTR2: {ratio_hctr2}%")
	print(f"AESpolyW/EME: {ratio_eme}%")

def table9_basics(x=8):
	print(f"Result for Table 9 (CA, CP, CAP, rAP)")
	CA = ext_value("hctr2-rijndael256.xctr.csv")
	CP = ext_value(f"hctr2-rijndael256.hashx{x}.csv")
	CAP = ext_value(f"hctr2-rijndael256.xctr‖hashx{x}.csv")
	if None in [CA, CP, CAP]:
		print("Not supported this experiment on this machine.")
		return
	rAP = round(CAP / (CA + CP), 3)
	
	print(f"CA: {CA}")
	print(f"CP: {CP}")
	print(f"CAP: {CAP}")
	print(f"rAP: {rAP}")

def table9_schemes(x=8):
	print(f"Result for Table 9 (performance of schemes)")
	HCTR2 = ext_value(f"hctr2-rijndael256.hctr2x{x}.csv")
	EME = ext_value(f"eme-rijndael256.emex{x}.csv")
	AESpolyW = ext_value(f"aespoly-rijndael256.aespolyx{x}.csv")
	if None in [HCTR2, EME, AESpolyW]:
		print("Not supported this experiment on this machine.")
		return
	ratio_hctr2 = round(AESpolyW/HCTR2, 3) * 100
	ratio_eme = round(AESpolyW/EME, 3) * 100

	print(f"HCTR2: {HCTR2}")
	print(f"EME: {EME}")
	print(f"AESpolyW: {AESpolyW}")
	print(f"AESpolyW/HCTR2: {ratio_hctr2}%")
	print(f"AESpolyW/EME: {ratio_eme}%")

def table10_basics(x=8):
	print(f"Result for Table 10 (CA, CP, CAP, rAP)")
	CA = ext_value("hctr2-simpira.xctr.csv")
	CP = ext_value(f"hctr2-simpira.hashx{x}.csv")
	CAP = ext_value(f"hctr2-simpira.xctr‖hashx{x}.csv")
	if None in [CA, CP, CAP]:
		print("Not supported this experiment on this machine.")
		return
	rAP = round(CAP / (CA + CP), 3)
	
	print(f"CA: {CA}")
	print(f"CP: {CP}")
	print(f"CAP: {CAP}")
	print(f"rAP: {rAP}")

def table10_schemes(x=8):
	print(f"Result for Table 10 (performance of schemes)")
	HCTR2 = ext_value(f"hctr2-simpira.hctr2x{x}.csv")
	EME = ext_value(f"eme-simpira.emex{x}.csv")
	AESpolyW = ext_value(f"aespoly-simpira.aespolyx{x}.csv")
	if None in [HCTR2, EME, AESpolyW]:
		print("Not supported this experiment on this machine.")
		return
	ratio_hctr2 = round(AESpolyW/HCTR2, 3) * 100
	ratio_eme = round(AESpolyW/EME, 3) * 100

	print(f"HCTR2: {HCTR2}")
	print(f"EME: {EME}")
	print(f"AESpolyW: {AESpolyW}")
	print(f"AESpolyW/HCTR2: {ratio_hctr2}%")
	print(f"AESpolyW/EME: {ratio_eme}%")

def table11(x=8):
	print(f"Result for Table 11")
	GMAC = ext_value(f"gmac-avx.gmacx{x}.csv")
	PMAC = ext_value(f"pmac-avx.pmacx{x}.csv")
	AESpolyM = ext_value(f"ecbpoly.ecbpolyx{x}.csv")
	if None in [GMAC, PMAC, AESpolyM]:
		print("Not supported this experiment on this machine.")
		return
	ratio_gmac = round(AESpolyM/GMAC, 3) * 100
	ratio_pmac = round(AESpolyM/PMAC, 3) * 100

	print(f"GMAC: {GMAC}")
	print(f"PMAC: {PMAC}")
	print(f"AESpolyM: {AESpolyM}")
	print(f"AESpolyM/GMAC: {ratio_gmac}%")
	print(f"AESpolyM/PMAC: {ratio_pmac}%")

def main():
	table4()
	table5()
	table6()
	table7()
	table9()
	table10()
	table11()

if __name__ == "__main__":
	main()