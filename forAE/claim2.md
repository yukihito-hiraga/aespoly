# Claim 2

On Intel processors, our schemes outperform other schemes except for HCTR2 instantiated with Rijndael-256.

## Relevance to the paper
This claim refers to the third contribution (Performance Evaluation) in Section 1.1 of the paper.
The relevant tables in the paper are Table 7, 9, 10, and 11.

## Script

Use `script2-x64.py` for Intel/AMD machines, and `script2-arm.py` for ARM machines.
`script2.py` is the same as `script2-x64.py` for Intel/AMD machines.
The script outputs results corresponding to the row corresponding to this machine's row of each table (7, 9, 10, and 11).

## Expected result

The script prints the performance results in the same format as the script for claim1.
The results vary for each run but are expected to reproduce values similar to the ones indicated by Intel-IL in Tables 7, 9, 10, and 11. 
