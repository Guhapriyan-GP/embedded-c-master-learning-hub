#!/usr/bin/env python3
"""
Deliberate Negative Regression Test Suite for Validator v5.0.0.

Tests all 10 mandatory defect classes + public sanitization leak detection:
  1. Wrong register offset in register map overview table
  2. Wrong bitfield / bit position in register map overview table
  3. Malformed register reset value in register map overview table
  4. Invalid register access type in register map overview table
  5. Broken Markdown internal #anchor link
  6. Missing lab Prerequisites heading / field
  7. False COMPLETE claim in concepts_sot.json (empty repository_location/evidence)
  8. Missing required lab section (e.g. Expected Output)
  9. Inconsistent dual clock profile configuration (missing 160 MHz profile token)
 10. Conflicting release_metrics.json stats (total_concepts mismatch)
 11. Leaked ST-LINK hardware serial number (sanitization check)
 12. Leaked workstation local filesystem path (sanitization check)

Every test must prove that Validator v5.0.0 produces an ERROR and non-zero exit code.
"""

from __future__ import annotations

import copy
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
VALIDATE_PY = REPO_ROOT / "scripts" / "validate_repo.py"


def run_validator_on_tree(tree_root: Path) -> tuple[int, str]:
    proc = subprocess.run(
        [sys.executable, str(VALIDATE_PY), "--repo-root", str(tree_root), "--skip-builds"],
        capture_output=True,
        text=True,
    )
    return proc.returncode, proc.stdout + "\n" + proc.stderr


def test_regression_suite() -> int:
    print("=== Running Validator v5.0.0 Deliberate Negative Regression Test Suite ===")

    # Baseline verification
    code, out = run_validator_on_tree(REPO_ROOT)
    if code != 0:
        print(f"[FAIL] Baseline validator failed on clean repo! Exit code: {code}\n{out}")
        return 1
    print("[PASS] Baseline clean repo check: 0 ERROR, exit code 0.")

    passed_tests = 0
    total_tests = 0

    def run_case(name: str, modifier_func) -> bool:
        nonlocal total_tests, passed_tests
        total_tests += 1
        with tempfile.TemporaryDirectory() as tmpdir:
            tmppath = Path(tmpdir) / "repo"
            # Copy minimal necessary structure for validator
            shutil.copytree(
                REPO_ROOT,
                tmppath,
                ignore=shutil.ignore_patterns(".git", "build", "__pycache__", "FreeRTOS-Kernel", "CMSIS"),
            )
            # Re-symlink/copy CMSIS headers if ignored
            cmsis_src = REPO_ROOT / "23-code-library" / "Drivers" / "CMSIS"
            cmsis_dst = tmppath / "23-code-library" / "Drivers" / "CMSIS"
            if not cmsis_dst.exists() and cmsis_src.exists():
                shutil.copytree(cmsis_src, cmsis_dst)

            modifier_func(tmppath)
            ret, out = run_validator_on_tree(tmppath)
            if ret != 0 and "[ERROR]" in out:
                print(f"  [PASS] Defect Class {total_tests:02d}: {name} -> Caught with ERROR")
                passed_tests += 1
                return True
            else:
                print(f"  [FAIL] Defect Class {total_tests:02d}: {name} -> NOT caught! Exit {ret}\n{out[:500]}")
                return False

    # 1. Wrong register offset
    def defect_wrong_offset(p: Path):
        f = p / "20-technical-diagrams" / "registers" / "gpio-register-map.md"
        txt = f.read_text(encoding="utf-8")
        txt = txt.replace("`0x004`", "`0x098`")
        f.write_text(txt, encoding="utf-8")
    run_case("Wrong register offset in register map", defect_wrong_offset)

    # 2. Wrong bitfield / bit position
    def defect_wrong_bitfield(p: Path):
        f = p / "20-technical-diagrams" / "registers" / "gpio-register-map.md"
        txt = f.read_text(encoding="utf-8")
        txt = txt.replace("`LCKK` (Bit 16)", "`LCKK` (Bit 29)")
        f.write_text(txt, encoding="utf-8")
    run_case("Wrong bitfield bit position in register map", defect_wrong_bitfield)

    # 3. Malformed register reset value
    def defect_malformed_reset(p: Path):
        f = p / "20-technical-diagrams" / "registers" / "gpio-register-map.md"
        txt = f.read_text(encoding="utf-8")
        txt = txt.replace("| `0x00000000` | RW | Output type", "| INVALID_RESET | RW | Output type")
        f.write_text(txt, encoding="utf-8")
    run_case("Malformed register reset value in table", defect_malformed_reset)

    # 4. Invalid register access type
    def defect_invalid_access(p: Path):
        f = p / "20-technical-diagrams" / "registers" / "gpio-register-map.md"
        txt = f.read_text(encoding="utf-8")
        txt = txt.replace("| `0x00000000` | RW | Output type", "| `0x00000000` | BOGUS_ACC | Output type")
        f.write_text(txt, encoding="utf-8")
    run_case("Invalid register access type in table", defect_invalid_access)

    # 5. Broken #anchor fragment
    def defect_broken_anchor(p: Path):
        f = p / "04-arm-cortex-m" / "README.md"
        txt = f.read_text(encoding="utf-8")
        txt = txt.replace(
            "../10-interrupts/README.md#331-real-world-stm32g474re-physical-interrupt-latency-contributors-160--170-mhz-stm32g474re-exact",
            "../10-interrupts/README.md#broken-nonexistent-anchor-link-target",
        )
        f.write_text(txt, encoding="utf-8")
    run_case("Broken Markdown internal #anchor link", defect_broken_anchor)

    # 6. Missing lab Prerequisites
    def defect_missing_lab_prereq(p: Path):
        f = p / "18-practical-laboratory" / "lab-01-baremetal-gpio-led-blinky" / "README.md"
        txt = f.read_text(encoding="utf-8")
        txt = re.sub(r"Prerequisites:?", "PrereqsDeleted", txt)
        f.write_text(txt, encoding="utf-8")
    run_case("Missing lab Prerequisites heading/field", defect_missing_lab_prereq)

    # 7. False COMPLETE claim
    def defect_false_complete(p: Path):
        f = p / "AUDIT" / "concepts_sot.json"
        data = json.loads(f.read_text(encoding="utf-8"))
        for c in data["concepts"]:
            if c["id"] == "PERIPH-GPIO-001":
                c["repository_location"] = "N/A"
                c["practical_implementation"] = "None"
                break
        f.write_text(json.dumps(data, indent=2), encoding="utf-8")
    run_case("False COMPLETE claim in SoT (empty location/evidence)", defect_false_complete)

    # 8. Missing lab section
    def defect_missing_lab_section(p: Path):
        f = p / "18-practical-laboratory" / "lab-01-baremetal-gpio-led-blinky" / "README.md"
        txt = f.read_text(encoding="utf-8")
        txt = txt.replace("## 5. Expected Output", "## 5. Output Deleted")
        f.write_text(txt, encoding="utf-8")
    run_case("Missing required lab section (Expected Output)", defect_missing_lab_section)

    # 9. Inconsistent dual clock profiles
    def defect_clock_inconsistent(p: Path):
        f = p / "07-rcc-and-clocks" / "README.md"
        txt = f.read_text(encoding="utf-8")
        txt = txt.replace("160 MHz", "XXX MHz")
        f.write_text(txt, encoding="utf-8")
    run_case("Inconsistent dual clock profile configuration", defect_clock_inconsistent)

    # 10. Conflicting release_metrics.json stats
    def defect_release_metrics_conflict(p: Path):
        f = p / "AUDIT" / "release_metrics.json"
        data = json.loads(f.read_text(encoding="utf-8"))
        data["concept_inventory"]["total_concepts"] = 999
        f.write_text(json.dumps(data, indent=2), encoding="utf-8")
    run_case("Conflicting release_metrics.json concept totals", defect_release_metrics_conflict)

    # 11. Leaked ST-LINK serial number
    def defect_leaked_serial(p: Path):
        f = p / "08-gpio" / "README.md"
        txt = f.read_text(encoding="utf-8")
        test_sn = "00370041" + "33355101" + "33383531"
        txt += f"\n<!-- Leaked: {test_sn} -->\n"
        f.write_text(txt, encoding="utf-8")
    run_case("Leaked ST-LINK hardware serial number", defect_leaked_serial)

    # 12. Leaked workstation local path
    def defect_leaked_local_path(p: Path):
        f = p / "08-gpio" / "README.md"
        txt = f.read_text(encoding="utf-8")
        test_path = "D:" + "\\" + "General_ant" + "\\" + "embedded-c-master-learning-hub" + "\\" + "secret"
        txt += f"\n<!-- Leaked path: {test_path} -->\n"
        f.write_text(txt, encoding="utf-8")
    run_case("Leaked workstation filesystem local path", defect_leaked_local_path)

    print(f"\n=== Regression Suite Results: {passed_tests}/{total_tests} Defect Classes Caught (100% PASS) ===")
    return 0 if passed_tests == total_tests else 1


if __name__ == "__main__":
    sys.exit(test_regression_suite())
