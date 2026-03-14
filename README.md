# AESpoly artifacts

Open-source implementations of the proposed methods (AESpolyW and AESpolyM) and the previous methods (HCTR2, EME, GMAC, and PMAC) instantiated with different BC primitives, along with the benchmarking codes.

## Instructions before executing scripts

Please follow these instructions in order.

### Requirements

Install these packages.

- python3 (>=3.12)
- python3-venv
- glib-2.0
	- install `libglib2.0-dev` package for debian users
	- install `glib2-devel` package for fedora users
- gcc (>= 13.3.0)

### To build

Make sure the current directory is the one containing this README.

- create venv
	- `python3 -m venv venv`
	- `source venv/bin/activate`
- then, run `./build.sh`

### To run benchmark

- run `sudo --preserve-env=PATH ./bench.sh`
	- make sure in venv
	- this takes about 10 minutes


## Claims

### Claim 1

Proposed metric rAP shows that AES-CLMUL parallelism is available on several processors, including the provided EC2 instance that runs on Intel Ice Lake microarchitecture.

#### Relevance to the paper
This claim refers to the first contribution (Instruction-Level Parallelism between the AES and
CLMUL Instructions) in Section 1.1 of the paper.
The relevant tables in the paper are Table 4, 5, 6, 9, and 10.

#### Script

`script1.py` outputs results corresponding to this machine's row in each of tables (Table 4, 5, 6, 9, and 10).

#### Expected result

The script shows the performance results on the console. 
The result consists of multiple parts corresponding to Table 4, 5, 6, 9, and 10.
The console output is self-explanatory and includes the lines as described below. 
```
Result for Table 4
CA: 0.323
CP: 0.275
CAP: 0.52
rAP: 0.87
```
These values should be similar to the ones on  Intel-IL’s row in the corresponding tables.


### Claim 2

On Intel processors, our schemes outperform other schemes except for HCTR2 instantiated with Rijndael-256.

#### Evidence
This claim refers to the first contribution (Performance Evaluation) in Section 1.1 of the paper.
The relevant tables in the paper are Table 7, Table 9, Table 10, and Table 11.

#### Script

Use `script2-x64.py` for Intel or AMD, and `script2-arm.py` for ARM.
The script outputs results corresponding to this machine's row of each table (7, 9, 10, and 11).

#### Expected result

The script prints the performance results in the same format as the script for claim1.
The results vary for each run but are expected to reproduce values similar to the ones indicated by Intel-IL in Tables 7, 9, 10, and 11. 

---

### Format of the raw results

Benchmark results are stored under the `result` folder.

Subfolders are named `result-<start_size>-<n>`, where `<start_size>` is the plaintext size of the first measurement and `<n>` is the number of measurements.
See `setting.yml` for details.

Each subfolder contains one or more CSV files with the following three columns: 
- the first means the size of plaintext (bytes)
- the second means the minimum cycles per byte (cpb)
- the last means the average cpb.

For example, row whose value is `2048,3.823864,3.988034` means:
- the size of plaintext is 2048 byte
- the minimum cpb is 3.823864
- the average cpb is 3.988034
