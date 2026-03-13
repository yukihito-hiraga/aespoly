from yaml import load, dump
try:
	from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
	from yaml import Loader, Dumper
from subprocess import run
from os import getuid
from pathlib import Path
from functools import reduce
from shutil import move
import numpy as np
import json

def get_cpuinfo():
	cpuinfo = []
	raw = []
	tmp = {}
	with open("/proc/cpuinfo") as f:
		raw = f.readlines()
	for e in raw:
		e = e.strip()
		if e == "":
			cpuinfo.append(tmp)
			tmp = {}
		else:
			key = e.split(":")[0].strip().strip("'")
			value = e.split(":")[1].strip().strip("'")
			if key == "processor":
				tmp["processor"] = int(value)
			if key == "cpu MHz":
				tmp["cpuMHz"] = float(value)
	return cpuinfo

def get_tscfreq():
	with open("/var/log/dmesg") as f:
		raw = f.readlines()
	Mhz = 0
	for l in raw:
		l = l.strip()
		l = reduce(lambda s,x:s+x, l.split(":")[1:]).strip()
		data = l.split(" ")
		sub = data[0]
		if sub == "tsc":
			print(data)
			if data[1] == "Detected" and data[4] == "processor":
				Mhz = float(data[2])
			if data[1] == "Detected" and data[4] == "TSC":
				Mhz = float(data[2])
			if data[1] == "Refined" and data[2] == "TSC":
				Mhz = float(data[5])
	return Mhz

def main():
	if getuid() != 0:
		print("please use sudo")
		return
	data = load(open("setting.yml"), Loader=Loader)
	targets = list(data["targets"])

	print("TSC {} Mhz".format(get_tscfreq()))
	
	dir_here = Path(__file__).parents[0]
	
	dir_root = Path(__file__).parents[2]
	
	avx512 = json.loads(open(dir_root/"build"/"meta.json").read())["avx512"]
	print(avx512)
	run(["rm -rf result/*"], shell=True)
	for s, t in data["benchset"]:
		foldername = "result-{}-{}".format(s, t)
		run(["mkdir",  "-p",  Path("result") / foldername])
		for target in targets:
			if "avx512" in target and not avx512:
				continue
			cpuinfo = get_cpuinfo()
			lfreq = np.array([ obj["cpuMHz"] for obj in cpuinfo ])
			pcore = lfreq.argmax()
			print(cpuinfo)
			print("cpu{} has the highest freq, {}".format(cpuinfo[pcore]["processor"], lfreq[pcore]))
			run(["sudo", dir_root / "build/{}".format(target), "{}".format(s), "{}".format(t), "{}".format(pcore), "{}".format(int(get_tscfreq() * 1000000)), "{}".format(int(lfreq[pcore] * 1000000))])
			for re in Path("result").iterdir():
				if re.is_file() and not(re.stem.split(".")[0] in targets):
					move(re, "result/{}.{}".format(target, re.name))
			run(["python", dir_here/"cpbgraph.py", target])
		run(["python", dir_here/"cpbgraph.py"])
		run("find result -maxdepth 1 -type f -exec mv {{}} result/{} \;".format(foldername), shell=True)
		#run(["mv -f result/* {}".format(foldername)], shell=True)
		#run(["mv -f {} result/{}".format(foldername, foldername)], shell=True)

main()