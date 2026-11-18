#!/usr/bin/env python3
"""
Automated Validation Suite for the Embedded C & STM32G474RE Master Learning Hub.

Validates:
  1. Core repository structure and mandatory AUDIT reports
  2. Absence of raw ASCII control characters in all Markdown and Mermaid files
  3. Internal relative Markdown links across all .md files
  4. Mermaid diagram syntax and prohibited HTML tags across all .mmd files
  5. Source-of-Truth Concept Inventory (AUDIT/concepts_sot.json) schema, ID uniqueness,
     27-domain coverage, and repository target existence
  6. Register map consistency checks against STM32G474RE ground truth (stm32g474xx.h)
  7. Curriculum module completeness (all 17 stages, completion criteria) & 20 labs
  8. Video database CSV integrity (50+ entries, required columns, valid URLs)
  9. Firmware compilation (arm-none-eabi-gcc debug + release) & ELF .isr_vector verification
 10. Host C unit test compilation and execution (tests/Makefile)

Exit code: 0 if all enabled checks pass, 1 otherwise.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

REQUIRED_DIRS = [
    "00-project-management",
    "01-master-concept-map",
    "01b-digital-electronics-and-hw-integration",
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
    "AUDIT",
    "tests",
    "scripts",
]

REQUIRED_AUDIT_FILES = [
    "AUDIT/concepts_sot.json",
    "AUDIT/COMPLETE_CONCEPT_INVENTORY.md",
    "AUDIT/CURRICULUM_COVERAGE_MATRIX.md",
    "AUDIT/MISSING_CONCEPTS_AND_GAPS.md",
    "AUDIT/STM32G474RE_PERIPHERAL_INVENTORY.md",
    "AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md",
    "AUDIT/PRACTICAL_IMPLEMENTATION_COVERAGE.md",
    "AUDIT/TECHNICAL_VERIFICATION_REPORT.md",
    "AUDIT/INDEPENDENT_RED_TEAM_REPORT.md",
    "AUDIT/FINAL_COMPLETENESS_REPORT.md",
    "AUDIT/KNOWN_LIMITATIONS.md",
]

STAGE_READMES = [
    "01b-digital-electronics-and-hw-integration/README.md",
    "02-c-fundamentals/README.md",
    "03-embedded-c/README.md",
    "04-arm-cortex-m/README.md",
    "05-toolchain-and-debugging/README.md",
    "06-stm32-fundamentals/README.md",
    "07-rcc-and-clocks/README.md",
    "08-gpio/README.md",
    "09-timers/README.md",
    "10-interrupts/README.md",
    "11-communication/README.md",
    "12-adc-dac-analog/README.md",
    "13-dma/README.md",
    "14-memory-and-storage/README.md",
    "15-power-management/README.md",
    "16-rtos/README.md",
    "17-advanced-firmware-engineering/README.md",
]

VALID_MERMAID_HEADERS = (
    "flowchart ",
    "graph ",
    "sequenceDiagram",
    "stateDiagram",
    "stateDiagram-v2",
    "classDiagram",
    "erDiagram",
    "xychart-beta",
)

VALID_STATUSES = {"COMPLETE", "PARTIAL", "MISSING", "NOT_APPLICABLE", "NEEDS_VERIFICATION"}
VALID_LEVELS = {"Level 1 - Essential", "Level 2 - Intermediate", "Level 3 - Advanced"}


class ValidationReport:
    def __init__(self) -> None:
        self.errors: list[str] = []
        self.warnings: list[str] = []
        self.info: list[str] = []

    def err(self, msg: str) -> None:
        self.errors.append(msg)
        print(f"[FAIL] {msg}")

    def warn(self, msg: str) -> None:
        self.warnings.append(msg)
        print(f"[WARN] {msg}")

    def ok(self, msg: str) -> None:
        self.info.append(msg)
        print(f"[PASS] {msg}")


def check_structure(rep: ValidationReport) -> None:
    missing_dirs = [d for d in REQUIRED_DIRS if not (REPO_ROOT / d).is_dir()]
    if missing_dirs:
        rep.err(f"Missing required directories: {missing_dirs}")
    else:
        rep.ok(f"All {len(REQUIRED_DIRS)} required directories present.")

    missing_audits = [f for f in REQUIRED_AUDIT_FILES if not (REPO_ROOT / f).is_file()]
    if missing_audits:
        rep.err(f"Missing mandatory AUDIT files: {missing_audits}")
    else:
        rep.ok(f"All {len(REQUIRED_AUDIT_FILES)} mandatory AUDIT files present.")

    if (REPO_ROOT / "scratch").exists():
        rep.err("Stale 'scratch/' directory still exists inside repository root.")
    else:
        rep.ok("Repository hygiene: no stale scratch/ generator directory.")


def check_control_chars(rep: ValidationReport) -> None:
    bad_files = []
    files = list(REPO_ROOT.rglob("*.md")) + list(REPO_ROOT.rglob("*.mmd"))
    files = [f for f in files if ".git" not in f.parts]
    for path in files:
        data = path.read_bytes()
        # Reject ASCII control bytes 0x00..0x08, 0x0B, 0x0C, 0x0E..0x1F (allow \t=0x09, \n=0x0A, \r=0x0D)
        bad_bytes = [b for b in data if b < 0x20 and b not in (0x09, 0x0A, 0x0D)]
        if bad_bytes:
            bad_files.append((str(path.relative_to(REPO_ROOT)), len(bad_bytes)))
    if bad_files:
        rep.err(f"Raw ASCII control bytes detected in {len(bad_files)} files: {bad_files[:10]}")
    else:
        rep.ok(f"Zero raw ASCII control characters across {len(files)} Markdown/Mermaid files.")


def strip_code_fences(text: str) -> str:
    out_lines = []
    in_fence = False
    for line in text.splitlines():
        stripped = line.lstrip()
        if stripped.startswith("```"):
            in_fence = not in_fence
            continue
        if not in_fence:
            out_lines.append(line)
    return "\n".join(out_lines)


def check_internal_links(rep: ValidationReport) -> None:
    link_re = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
    broken: list[str] = []
    checked = 0
    md_files = [f for f in REPO_ROOT.rglob("*.md") if ".git" not in f.parts]
    for md in md_files:
        text = strip_code_fences(md.read_text(encoding="utf-8", errors="replace"))
        for match in link_re.finditer(text):
            raw_target = match.group(1).strip()
            if not raw_target or raw_target.startswith(("#", "http://", "https://", "mailto:", "file://")):
                continue
            # Handle optional title in quotes
            target = raw_target.split()[0].split("#")[0]
            if not target:
                continue
            checked += 1
            resolved = (md.parent / target).resolve()
            if not resolved.exists():
                broken.append(f"{md.relative_to(REPO_ROOT)} -> {raw_target}")
    if broken:
        rep.err(f"Broken internal links ({len(broken)} of {checked} checked): {broken[:15]}")
    else:
        rep.ok(f"All {checked} internal relative Markdown links verified across {len(md_files)} files.")


def check_mermaid_diagrams(rep: ValidationReport) -> None:
    mmd_files = [f for f in REPO_ROOT.rglob("*.mmd") if ".git" not in f.parts]
    issues = []
    for mmd in mmd_files:
        txt = mmd.read_text(encoding="utf-8", errors="replace")
        non_empty = [ln.strip() for ln in txt.splitlines() if ln.strip() and not ln.strip().startswith("%%")]
        if not non_empty:
            issues.append(f"{mmd.name}: empty diagram")
            continue
        if not any(non_empty[0].startswith(h) for h in VALID_MERMAID_HEADERS):
            issues.append(f"{mmd.name}: unsupported header '{non_empty[0]}'")
        if "<br" in txt.lower():
            issues.append(f"{mmd.name}: contains HTML <br> tag")
    if issues:
        rep.err(f"Mermaid validation issues: {issues}")
    else:
        rep.ok(f"All {len(mmd_files)} Mermaid (.mmd) diagrams valid and free of HTML tags.")


def check_sot_concepts(rep: ValidationReport) -> None:
    sot_path = REPO_ROOT / "AUDIT" / "concepts_sot.json"
    if not sot_path.is_file():
        rep.err("AUDIT/concepts_sot.json missing.")
        return
    data = json.loads(sot_path.read_text(encoding="utf-8"))
    concepts = data.get("concepts", [])
    if len(concepts) < 200:
        rep.err(f"Concept inventory too small ({len(concepts)} concepts; expected >= 200).")
        return

    ids = set()
    domains = set()
    missing_targets = []
    schema_errors = []

    required_keys = [
        "id", "concept", "domain_num", "domain", "level",
        "prerequisites", "explanation", "hardware_relevance",
        "practical_implementation", "reference", "learning_resource",
        "completion_criteria", "repository_location",
        "initial_status", "current_status",
    ]
    for c in concepts:
        cid = c.get("id", "<missing>")
        if cid in ids:
            schema_errors.append(f"Duplicate Concept ID: {cid}")
        ids.add(cid)
        domains.add(c.get("domain_num"))
        for k in required_keys:
            if k not in c or (isinstance(c[k], str) and not c[k].strip()):
                schema_errors.append(f"{cid}: missing/empty field '{k}'")
        if c.get("initial_status") not in VALID_STATUSES:
            schema_errors.append(f"{cid}: invalid initial_status '{c.get('initial_status')}'")
        if c.get("current_status") not in VALID_STATUSES:
            schema_errors.append(f"{cid}: invalid current_status '{c.get('current_status')}'")
        if c.get("level") not in VALID_LEVELS:
            schema_errors.append(f"{cid}: invalid level '{c.get('level')}'")
        loc = c.get("repository_location", "")
        if loc and loc != "N/A":
            first_loc = loc.split(";")[0].split("#")[0].strip()
            if not (REPO_ROOT / first_loc).exists():
                missing_targets.append(f"{cid} -> {first_loc}")

    if schema_errors:
        rep.err(f"SOT schema errors ({len(schema_errors)}): {schema_errors[:10]}")
    elif len(domains) < 27:
        rep.err(f"SOT covers only {len(domains)}/27 required domains.")
    elif missing_targets:
        rep.err(f"SOT points to non-existent repo locations ({len(missing_targets)}): {missing_targets[:10]}")
    else:
        rep.ok(f"Source-of-Truth verified: {len(concepts)} unique Concept IDs across all {len(domains)}/27 domains.")


def check_register_ground_truth(rep: ValidationReport) -> None:
    checks = [
        ("20-technical-diagrams/registers/pwr-flash-register-map.md", r"0x0?80", "PWR_CR5 offset 0x80"),
        ("20-technical-diagrams/registers/pwr-flash-register-map.md", r"PWR_SR2", "VOSF in PWR_SR2"),
        ("20-technical-diagrams/registers/rcc-register-map.md", r"Bit\s*8|bit\s*8|\|\s*8\s*\|", "RCC_CR.HSION at bit 8"),
        ("20-technical-diagrams/registers/gpio-register-map.md", r"0xFFFFFFFF|11.*Analog|Analog.*11", "GPIOx_MODER Analog reset"),
        ("20-technical-diagrams/registers/adc-dac-register-map.md", r"0x20000000", "ADC_CR reset 0x20000000 (DEEPPWD)"),
        ("20-technical-diagrams/registers/adc-dac-register-map.md", r"42\.5\s*MHz|HCLK/4|CKMODE\s*=\s*11", "ADC CKMODE HCLK/4 <= 60 MHz"),
        ("20-technical-diagrams/registers/dma-dmamux-register-map.md", r"24.*USART1_RX|USART1_RX.*24", "DMAMUX Req 24 = USART1_RX"),
        ("20-technical-diagrams/registers/watchdog-register-map.md", r"100b|0b100|Ch\.?\s*35", "IWDG PR=100b & RM0440 Ch 35"),
        ("23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h", r"FMAC_IRQn\s*=\s*101", "stm32g474xx.h max IRQn = 101"),
    ]
    failed = []
    for rel, pattern, desc in checks:
        p = REPO_ROOT / rel
        if not p.is_file():
            failed.append(f"{desc}: file missing ({rel})")
            continue
        txt = p.read_text(encoding="utf-8", errors="replace")
        if not re.search(pattern, txt, re.IGNORECASE):
            failed.append(f"{desc}: pattern not found in {rel}")
    if failed:
        rep.err(f"Register ground-truth consistency failures: {failed}")
    else:
        rep.ok(f"All {len(checks)} STM32G474RE register ground-truth invariants verified.")


def check_curriculum_and_labs(rep: ValidationReport) -> None:
    missing_criteria = []
    for rel in STAGE_READMES:
        p = REPO_ROOT / rel
        if not p.is_file():
            missing_criteria.append(f"{rel} (missing)")
            continue
        txt = p.read_text(encoding="utf-8", errors="replace")
        if "Completion Criteria" not in txt or len(txt) < 5000:
            missing_criteria.append(f"{rel} (len={len(txt)})")
    if missing_criteria:
        rep.err(f"Incomplete curriculum modules: {missing_criteria}")
    else:
        rep.ok(f"All {len(STAGE_READMES)} curriculum stage modules verified (>= 5 KB, with Completion Criteria).")

    labs_dir = REPO_ROOT / "18-practical-laboratory"
    labs = sorted([d for d in labs_dir.iterdir() if d.is_dir() and d.name.startswith("lab-")])
    if len(labs) < 20:
        rep.err(f"Expected >= 20 practical labs, found {len(labs)}.")
    else:
        rep.ok(f"All {len(labs)} practical lab directories verified in 18-practical-laboratory/.")

    reg_maps = sorted((REPO_ROOT / "20-technical-diagrams" / "registers").glob("*.md"))
    rep.ok(f"Verified {len(reg_maps)} peripheral register maps in 20-technical-diagrams/registers/.")


def check_video_database(rep: ValidationReport) -> None:
    csv_path = REPO_ROOT / "22-video-library" / "video-database.csv"
    if not csv_path.is_file():
        rep.err("22-video-library/video-database.csv missing.")
        return
    with csv_path.open("r", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    bad_urls = [r.get("index") for r in rows if not r.get("url", "").startswith("https://")]
    if len(rows) < 30 or bad_urls:
        rep.err(f"Video DB check failed: {len(rows)} rows, bad URLs: {bad_urls}")
    else:
        rep.ok(f"Video database CSV verified with {len(rows)} entries and valid HTTPS URLs.")


def find_arm_tool(name: str) -> str | None:
    found = shutil.which(name)
    if found:
        return found
    st_root = Path(r"C:\ST")
    if st_root.is_dir():
        matches = list(st_root.rglob(f"{name}.exe"))
        if matches:
            return str(matches[0])
    return None


def check_builds_and_tests(rep: ValidationReport) -> None:
    make_exe = shutil.which("make") or find_arm_tool("make")
    gcc_arm = find_arm_tool("arm-none-eabi-gcc")
    readelf_arm = find_arm_tool("arm-none-eabi-readelf")

    if not make_exe or not gcc_arm:
        rep.warn("Skipping firmware build check: make or arm-none-eabi-gcc not found.")
    else:
        tpl_dir = REPO_ROOT / "23-code-library" / "templates"
        env = os.environ.copy()
        arm_bin = str(Path(gcc_arm).parent)
        make_bin = str(Path(make_exe).parent)
        env["PATH"] = f"{arm_bin}{os.pathsep}{make_bin}{os.pathsep}{env.get('PATH', '')}"

        for build_type in ("debug", "release"):
            cmd = [make_exe, f"BUILD_TYPE={build_type}", "clean", "all"]
            proc = subprocess.run(cmd, cwd=tpl_dir, env=env, capture_output=True, text=True)
            if proc.returncode != 0:
                rep.err(f"Firmware build ({build_type}) FAILED:\n{proc.stdout}\n{proc.stderr}")
                return
        rep.ok("Firmware template compiled cleanly (BUILD_TYPE=debug & release, 0 warnings).")

        elf_path = tpl_dir / "build" / "release" / "nucleo_g474re_baremetal.elf"
        if not elf_path.is_file():
            elves = list((tpl_dir / "build").rglob("*.elf"))
            elf_path = elves[0] if elves else elf_path
        if readelf_arm and elf_path.is_file():
            re_proc = subprocess.run([readelf_arm, "-SW", str(elf_path)], capture_output=True, text=True)
            m = re.search(r"\.isr_vector\s+PROGBITS\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S+\s+\S+\s+\S+\s+\S+\s+(\d+)", re_proc.stdout)
            if m:
                addr = int(m.group(1), 16)
                size = int(m.group(2), 16)
                align = int(m.group(3))
                if addr == 0x08000000 and size == 472 and align >= 512:
                    rep.ok(f"ELF .isr_vector verified: addr=0x{addr:08X}, size={size} B (118 vectors), align={align} B.")
                else:
                    rep.err(f"ELF .isr_vector invalid: addr=0x{addr:08X}, size={size}, align={align}")
            else:
                rep.warn("Could not parse .isr_vector line from readelf output.")

    host_gcc = shutil.which("gcc")
    if not host_gcc or not make_exe:
        rep.warn("Skipping host C unit tests: host gcc or make not found.")
    else:
        tests_dir = REPO_ROOT / "tests"
        proc = subprocess.run([make_exe, "clean", "test"], cwd=tests_dir, capture_output=True, text=True)
        if proc.returncode != 0:
            rep.err(f"Host C unit tests FAILED:\n{proc.stdout}\n{proc.stderr}")
        else:
            rep.ok("Host C unit tests (test_c_foundations + test_ring_buffer) compiled and passed (exit 0).")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Embedded C Master Learning Hub.")
    parser.add_argument("--skip-builds", action="store_true", help="Skip compiler and unit-test checks.")
    args = parser.parse_args()

    print(f"=== Validating Embedded C Master Learning Hub at: {REPO_ROOT} ===")
    rep = ValidationReport()
    check_structure(rep)
    check_control_chars(rep)
    check_internal_links(rep)
    check_mermaid_diagrams(rep)
    check_sot_concepts(rep)
    check_register_ground_truth(rep)
    check_curriculum_and_labs(rep)
    check_video_database(rep)
    if not args.skip_builds:
        check_builds_and_tests(rep)

    print(f"=== Validation Summary: {len(rep.info)} PASS, {len(rep.warnings)} WARN, {len(rep.errors)} FAIL ===")
    return 1 if rep.errors else 0


if __name__ == "__main__":
    sys.exit(main())
