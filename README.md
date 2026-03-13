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

## Claims

### Claim 1

Proposed metric rAP shows that AES-CLMUL parallelism is available on several processors, including those whose microarchtecure is Sapphire Rapids (Intel).

#### Evidence
This claim refers to the first contribution (Instruction-Level Parallelism between the AES and
CLMUL Instructions) in Section 1.1 of the paper.
The relevant tables in the paper are Table 4, Table 5, Table 6, Table 9, and Table 10.

#### Script

`script1.py` outputs results corresponding to this machine's row in each of tables (Table 4, 5, 6, 9, and 10).

#### Expected result

All values are expected to be close to the corresponding values appeared in the paper.
For instance, all rAP are expected to be less than 1, and AES-OCB/AES-GCM is expected to be less than 100%.

You can verify the correctness of the values of rAP by compute rAP as CAP/(CA + CP) using CA, CP, and CAP values also reported by this script.


### Claim 2

Our proposed schemes AESpolyW and AESpolyM take advantage of AES-CLMUL parallelism to achieve better performance than prior schemes.

#### Evidence
This claim refers to the first contribution (Performance Evaluation) in Section 1.1 of the paper.
The relevant tables in the paper are Table 7, Table 9, Table 10, and Table 11.

#### Script

Use `script2-x64.py` for Intel or AMD, and `script2-arm.py` for ARM.
The script outputs results corresponding to this machine's row of each table (7, 9, 10, 11) described above.

#### Expected result

All values are expected to be close to the corresponding value appeared in the paper.
Although the cpb of our scheme is larger than that of another scheme on some processor (such as AESpolyM on AMD processors), our shemes usually outperform the other schemes overall.


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