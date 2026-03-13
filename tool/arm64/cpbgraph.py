import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import csv
from pathlib import Path
from sys import argv

matplotlib.use('Agg')

dirfix = "./"

def csv2npar(filename : str):
	res = []
	with open(filename) as f:
		c = csv.reader(f, delimiter=",")
		res = np.array([ list(map(lambda x:float(x), r)) for r in c ])
	return res

fig_avg = plt.figure()
fig_min = plt.figure()

ax_avg = fig_avg.add_subplot()
ax_min = fig_min.add_subplot()

def add_data(path : Path):
	data = csv2npar(path)
	name = path.stem
	ax_avg.plot(data.transpose()[0], data.transpose()[2], label=name)
	ax_min.plot(data.transpose()[0], data.transpose()[1], label=name)

def save(path : Path, name : str):
	fig_avg.legend()
	ax_avg.set_title("Average cpb")
	ax_avg.set_xlabel("bytes of plaintext")
	ax_avg.set_ylabel("cpb")
	ax_avg.set_ylim(bottom=0)
	fig_avg.savefig(path / (name + "avg-cpb.png"))

	fig_min.legend()
	ax_min.set_title("Min cpb")
	ax_min.set_xlabel("bytes of plaintext")
	ax_min.set_ylabel("cpb")
	ax_min.set_ylim(bottom=0)
	fig_min.savefig(path / (name + "min-cpb.png"))

def main():
	path = Path("./result")
	list = []
	argc = len(argv)
	if argc > 1:
		list = argv[1].split(",")
	for e in path.iterdir():
		if e.is_file() and e.suffix == ".csv":
			if argc > 1 and e.stem.split(".")[0] in list:
				add_data(e)
			elif argc < 2:
				add_data(e)
	if argc > 1:
		save(path, argv[1].replace(",", "-") + ".")
	else:
		save(path, "")


main()