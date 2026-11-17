import os
import re
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

print(f"=== Validating Embedded C Master Learning Hub at: {REPO_ROOT} ===")

expected_dirs = [
    "00-project-management",
    "01-master-concept-map",
    "02-c-fundamentals",
    "03-embedded-c",
    "04-arm-cortex-m",
    "05-toolchain-and-debugging",
    "06-stm32-fundamentals",
    "07-rcc-and-clocks",
    "08-gpio",
    "09-timers",
    "10-interrupts",
    "11-communication",
    "12-adc-dac-analog",
    "13-dma",
    "14-memory-and-storage",
    "15-power-management",
    "16-rtos",
    "17-advanced-firmware-engineering",
    "18-practical-laboratory",
    "19-projects",
    "20-technical-diagrams",
    "21-reference-library",
    "22-video-library",
    "23-code-library",
    "24-assessments",
    "25-glossary",
    "scripts"
]

missing_dirs = []
for d in expected_dirs:
    dir_path = os.path.join(REPO_ROOT, d)
    if not os.path.isdir(dir_path):
        missing_dirs.append(d)

if missing_dirs:
    print(f"[FAIL] Missing directories: {missing_dirs}")
else:
    print(f"[PASS] All {len(expected_dirs)} core directories present.")

# Check video database
csv_path = os.path.join(REPO_ROOT, "22-video-library", "video-database.csv")
if os.path.isfile(csv_path):
    with open(csv_path, "r", encoding="utf-8") as f:
        lines = f.readlines()
    print(f"[PASS] Video database CSV present with {len(lines) - 1} entries.")
else:
    print("[FAIL] Missing video-database.csv")

# Count markdown files
md_files = []
for root, _, files in os.walk(REPO_ROOT):
    for f in files:
        if f.endswith(".md"):
            md_files.append(os.path.join(root, f))

print(f"[INFO] Found {len(md_files)} markdown documentation files across repository.")

# Check practical labs
labs_dir = os.path.join(REPO_ROOT, "18-practical-laboratory")
labs_found = [d for d in os.listdir(labs_dir) if os.path.isdir(os.path.join(labs_dir, d))]
print(f"[PASS] Found {len(labs_found)} practical lab directories in 18-practical-laboratory.")

print("=== Repository Validation Complete ===")
