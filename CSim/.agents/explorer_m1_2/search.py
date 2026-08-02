import os
import re

source_dir = r"c:\Users\gravi\Source\Projects\CSim - Copy\CSim\Source"
output_file = r"c:\Users\gravi\Source\Projects\CSim - Copy\CSim\.agents\explorer_m1_2\search_output.txt"

patterns = {
    "EntityTable": re.compile(r"\bEntityTable\b"),
    "EntityID": re.compile(r"\bEntityID\b"),
    "Camera": re.compile(r"\bCamera\b"),
    "nodeLookup": re.compile(r"\bnodeLookup\b"),
    "window": re.compile(r"\bwindow\b"),
}

results = []

for root, dirs, files in os.walk(source_dir):
    for file in files:
        if file.endswith((".h", ".cpp")):
            path = os.path.join(root, file)
            try:
                with open(path, "r", encoding="utf-8", errors="ignore") as f:
                    lines = f.readlines()
                for idx, line in enumerate(lines):
                    for name, pat in patterns.items():
                        if pat.search(line):
                            results.append(f"{file}:{idx+1} ({name}): {line.strip()}")
            except Exception as e:
                results.append(f"Error reading {file}: {str(e)}")

with open(output_file, "w", encoding="utf-8") as f:
    f.write("\n".join(results))
print("Done")
