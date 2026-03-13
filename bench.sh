#!/bin/bash
if [ "$EUID" -ne 0 ]; then
	echo "please use sudo or execute as root"
else
	path_smt=/sys/devices/system/cpu/smt/control
	#hyperthreading off
	if [[ -e $path_smt && $(<"$path_smt") =~ ^(on|off)$ ]]; then
		printf off > "$path_smt"
	fi

	path_turbo=/sys/devices/system/cpu/intel_pstate/no_turbo
	if [ -e "$path_turbo" ]; then
		tb=$(cat "$path_turbo")
		#turbo boost off
		echo "1" | sudo tee "$path_turbo"
	fi

	mkdir -p result

	python tool/bench.py

	#hyperthreading on
	if [[ -e $path_smt && $(<"$path_smt") =~ ^(on|off)$ ]]; then
		printf on > "$path_smt"
	fi

	if [ -e "$path_turbo" ]; then
		#turbo boost on
		echo $tb | sudo tee "$path_turbo"
	fi
fi