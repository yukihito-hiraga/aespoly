import json
from pathlib import Path
from subprocess import run

def main():
	dir = Path(__file__).parents[1]
	with open(dir/"build"/"meta.json") as f:
		obj = json.loads(f.read())
		if "arch" in obj.keys():
			if obj["arch"] == '"aarch64"':
				print(obj["is_apple_silicon"])
				run(f"python {dir/f"tool/arm64/bench-apple.py" if obj["is_apple_silicon"] else dir/f"tool/arm64/bench-notapple.py"}", shell=True)
			if obj["arch"] == '"x86_64"':
				run(f"python {dir/"tool/x64/bench.py"}", shell=True)

if __name__ == "__main__":
	main()