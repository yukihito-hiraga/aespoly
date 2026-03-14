# Claim 1

Proposed metric rAP shows that AES-CLMUL parallelism is available on several processors, including the provided EC2 instance that runs on Intel Ice Lake microarchitecture.

## Relevance to the paper
This claim refers to the first contribution (Instruction-Level Parallelism between the AES and
CLMUL Instructions) in Section 1.1 of the paper.
The relevant tables in the paper are Table 4, 5, 6, 9, and 10.

## Script

`script1.py` outputs results corresponding to this machine's row in each of tables (Table 4, 5, 6, 9, and 10).

## Expected result

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
