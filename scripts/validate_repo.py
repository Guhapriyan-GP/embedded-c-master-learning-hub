#!/usr/bin/env python3
"""
Regression-Resistant Automated Validation Suite for the
Embedded C & STM32G474RE Master Learning Hub (v4.2.0).

Validates all 10 mandatory Section 2.C verification pillars + Round-2 Section 4 hardening:
  1. Core repository structure, governance/AUDIT files (incl. Section 9 reports), and raw ASCII control bytes
  2. Real CMSIS header cross-checking (programmatic parsing of stm32g474xx.h &
     core_cm4.h base addresses, C typedef struct byte offsets, IRQn_Type, and
     _Pos/_Msk bitfields cross-verified directly against all 16 register maps —
     including bullet bit positions, table-cell FIELD[hi:lo] / (bN) ranges,
     official RM0440 / PM0214 / ARMv7-M citation checks, and explicit scope transparency)
  3. Evidence-based Source-of-Truth Concept Audit (AUDIT/concepts_sot.json):
     350 unique IDs, 27/27 domains, 0 cycles, 0 unknown prereqs, 0 inverted-level
     edges, substantive file/field/hardware-status evidence for every COMPLETE concept,
     never auto-promoting PARTIAL -> COMPLETE, plus transparent MANUAL REVIEW queue
  4. Practical Lab 11-section + Prerequisites + Clock Profile + Standardized
     Verification Record audit across all 20 labs in 18-practical-laboratory/lab-*/README.md
  5. Local Markdown Link, Internal #anchor Fragment (GFM heading slugification),
     Reference-Style Link, Image, and Expanded GitHub MathJax Safety audit —
     cleanly separated from live External URL probing (which tracks 2xx, 3xx redirects
     to final URL, 403/429 bot-blocks, 404/410 broken links, and offline resilience)
  6. Mermaid (.mmd) Syntax vs Render separation: full Python structural syntax parser
     (mermaid_syntax) PLUS runtime probe for 'mmdc' (@mermaid-js/mermaid-cli) rendering
     (mermaid_render -> Successfully rendered or explicit UNVERIFIED when absent)
  7. Curriculum 13-section + authoritative primary reference + Prev/Next navigation
     audit across all 17 stages (01b + 02-17), plus 12 drivers, FreeRTOS V11.1.0,
     3 Capstones, and strict Video CSV schema/ID/URL integrity
  8. Non-short-circuiting Firmware & C-Snippet Builds + ELF .isr_vector verification
     using the exact 9-state build/test reporting taxonomy (TOOLCHAIN_UNAVAILABLE,
     BUILD_SKIPPED, BUILD_FAILED, BUILD_PASSED, ELF_INSPECTION_UNAVAILABLE,
     ELF_INSPECTION_PASSED, HOST_TESTS_SKIPPED, HOST_TESTS_FAILED, HOST_TESTS_PASSED)
  9. Independent Host C unit tests (make -C tests clean test -> 0 failed, >= 500
     assertions, always cleaned up afterward with make -C tests clean)
 10. Structured Severity Taxonomy (INFO/PASS, WARNING, UNVERIFIED, ERROR) with
     optional JSON report export (--json-report <path>)

Exit code: 1 if ERROR > 0, else 0.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import csv
import json
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

VALIDATOR_VERSION = "4.2.0"
DEFAULT_REPO_ROOT = Path(__file__).resolve().parent.parent

EXCLUDED_DIR_NAMES = {
    ".git",
    "FreeRTOS-Kernel",
    "CMSIS",
    "build",
    "__pycache__",
    ".pytest_cache",
    "node_modules",
}

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
    "BACKUP",
    "HARDWARE_VALIDATION",
    "LEARNERS",
    "PROJECT_MANAGEMENT",
    "tests",
    "scripts",
]

REQUIRED_ROOT_AND_SUPPORT_FILES = [
    "START_HERE.md",
    "Doxyfile",
    ".clang-tidy",
    ".clang-format",
    "BACKUP/BACKUP_MANIFEST.md",
    "PROJECT_MANAGEMENT/REMEDIATION_TRACKER.md",
    "HARDWARE_VALIDATION/PENDING_EXTERNAL_CONNECTIONS.md",
    "LEARNERS/BEGINNER_SETUP_WINDOWS11.md",
    "LEARNERS/LEARNER_PROGRESS_DASHBOARD.md",
    "LEARNERS/WORKED_EXAMPLES.md",
    "LEARNERS/TROUBLESHOOTING_GUIDE.md",
    "LEARNERS/FORMULA_AND_CALCULATION_SHEET.md",
    "LEARNERS/REGISTER_QUICK_REFERENCE.md",
    "LEARNERS/REVISION_NOTES_AND_MASTERY_GATES.md",
]

REQUIRED_AUDIT_FILES = [
    "AUDIT/concepts_sot.json",
    "AUDIT/COMPLETE_CONCEPT_INVENTORY.md",
    "AUDIT/CURRICULUM_COVERAGE_MATRIX.md",
    "AUDIT/UPDATED_CONCEPT_COVERAGE_MATRIX.md",
    "AUDIT/MISSING_CONCEPTS_AND_GAPS.md",
    "AUDIT/MISSING_CONCEPTS_RESOLUTION_REPORT.md",
    "AUDIT/PARTIAL_CONCEPT_COMPLETION_REPORT.md",
    "AUDIT/STM32G474RE_PERIPHERAL_INVENTORY.md",
    "AUDIT/PREREQUISITE_DEPENDENCY_AUDIT.md",
    "AUDIT/PRACTICAL_IMPLEMENTATION_COVERAGE.md",
    "AUDIT/TECHNICAL_VERIFICATION_REPORT.md",
    "AUDIT/INDEPENDENT_RED_TEAM_REPORT.md",
    "AUDIT/FINAL_COMPLETENESS_REPORT.md",
    "AUDIT/KNOWN_LIMITATIONS.md",
    "AUDIT/UPDATED_KNOWN_LIMITATIONS.md",
    "AUDIT/HARDWARE_VALIDATION_SUMMARY.md",
    "AUDIT/FREERTOS_VERIFICATION_REPORT.md",
    "AUDIT/DRIVER_LIBRARY_VERIFICATION.md",
    "AUDIT/TEST_COVERAGE_REPORT.md",
    "AUDIT/VIDEO_COVERAGE_REPORT.md",
    "AUDIT/CAPSTONE_IMPLEMENTATION_REPORT.md",
    "AUDIT/LEARNING_EXPERIENCE_REVIEW.md",
    "AUDIT/MASTER_REMEDIATION_TRACKER.md",
    "AUDIT/TECHNICAL_DEFECTS.md",
    "AUDIT/VALIDATOR_QUALITY_REPORT.md",
    "AUDIT/LAB_VERIFICATION_MATRIX.md",
    "AUDIT/HARDWARE_VERIFICATION_MATRIX.md",
    "AUDIT/EDUCATIONAL_QUALITY_REPORT.md",
    "AUDIT/ROUND_2_FINDINGS.md",
    "AUDIT/ROUND_2_REMEDIATION_REPORT.md",
    "AUDIT/ROUND_2_VALIDATION_RESULTS.md",
    "AUDIT/FINAL_REMEDIATION_REPORT.md",
]

REQUIRED_DRIVERS = [
    "clock_g4",
    "gpio_g4",
    "ring_buffer",
    "fault_handler",
    "exti_g4",
    "tim_g4",
    "usart_g4",
    "spi_g4",
    "i2c_g4",
    "adc_g4",
    "dma_g4",
    "iwdg_g4",
]

CAPSTONE_DIRS = [
    "19-projects/01-beginner-smart-instrument-panel",
    "19-projects/02-intermediate-dma-data-logger",
    "19-projects/03-advanced-freertos-digital-power-controller",
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

REQUIRED_CMSIS_BASES = [
    "RCC_BASE",
    "PWR_BASE",
    "FLASH_R_BASE",
    "GPIOA_BASE",
    "EXTI_BASE",
    "SYSCFG_BASE",
    "TIM1_BASE",
    "TIM2_BASE",
    "USART2_BASE",
    "SPI1_BASE",
    "I2C1_BASE",
    "ADC1_BASE",
    "DAC1_BASE",
    "DMA1_BASE",
    "DMAMUX1_BASE",
    "IWDG_BASE",
    "WWDG_BASE",
    "FDCAN1_BASE",
    "HRTIM1_BASE",
    "CORDIC_BASE",
    "FMAC_BASE",
    "CRC_BASE",
    "RNG_BASE",
    "OPAMP1_BASE",
    "COMP1_BASE",
    "RTC_BASE",
    "TAMP_BASE",
    "USB_BASE",
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
VALID_HW_STATUSES = {
    "ONBOARD_HARDWARE_TESTED",
    "HOST_TESTED",
    "CROSS_COMPILED",
    "EXTERNAL_HARDWARE_PENDING",
    "NEEDS_BENCH_INSTRUMENT",
    "NOT_APPLICABLE",
}
VALID_LEVELS = {
    "Level 1 - Essential": 1,
    "Level 2 - Intermediate": 2,
    "Level 3 - Advanced": 3,
}

PLACEHOLDER_STRINGS = {"TBD", "TODO", "N/A", "NA", "NONE", "WIP", "FIXME", "...", "-"}

STOPWORDS = {
    "and", "the", "for", "with", "from", "into", "over", "under", "using", "via",
    "mode", "modes", "basic", "advanced", "basics", "overview", "design", "pattern",
    "system", "embedded", "hardware", "software", "stm32", "stm32g4", "stm32g474re",
    "arm", "cortex", "level", "data", "control", "config", "configuration", "bit",
    "bits", "byte", "bytes", "register", "registers", "generic", "common", "type",
    "types", "based", "multi", "single", "high", "low", "fast", "safe", "safety",
}

C_TYPE_SIZES: dict[str, tuple[int, int]] = {
    "uint32_t": (4, 4),
    "int32_t": (4, 4),
    "uint16_t": (2, 2),
    "int16_t": (2, 2),
    "uint8_t": (1, 1),
    "int8_t": (1, 1),
}

HTML_TAG_RE = re.compile(
    r"</?(?:br|b|i|u|em|strong|span|div|p|a|font|sub|sup|code|pre|hr|img|table|tr|td|th)\b[^>]*>",
    re.IGNORECASE,
)


@dataclass
class Finding:
    severity: str  # "INFO", "WARNING", "UNVERIFIED", "ERROR"
    check: str
    message: str
    details: list[str] = field(default_factory=list)


class ValidationReport:
    def __init__(self, quiet: bool = False) -> None:
        self.quiet = quiet
        self.findings: list[Finding] = []
        self.metrics: dict[str, Any] = {}

    @property
    def errors(self) -> list[Finding]:
        return [f for f in self.findings if f.severity == "ERROR"]

    @property
    def warnings(self) -> list[Finding]:
        return [f for f in self.findings if f.severity == "WARNING"]

    @property
    def unverified(self) -> list[Finding]:
        return [f for f in self.findings if f.severity == "UNVERIFIED"]

    @property
    def infos(self) -> list[Finding]:
        return [f for f in self.findings if f.severity == "INFO"]

    def _emit(self, sev: str, check: str, msg: str, details: list[str] | None = None) -> None:
        f = Finding(severity=sev, check=check, message=msg, details=details or [])
        self.findings.append(f)
        if not self.quiet:
            tag = {"INFO": "INFO/PASS", "WARNING": "WARNING", "UNVERIFIED": "UNVERIFIED", "ERROR": "ERROR"}[sev]
            print(f"[{tag}] [{check}] {msg}")
            if details:
                for d in details[:12]:
                    print(f"    - {d}")
                if len(details) > 12:
                    print(f"    ... (+{len(details) - 12} more)")

    def info(self, check: str, msg: str, details: list[str] | None = None) -> None:
        self._emit("INFO", check, msg, details)

    def warn(self, check: str, msg: str, details: list[str] | None = None) -> None:
        self._emit("WARNING", check, msg, details)

    def unverified_item(self, check: str, msg: str, details: list[str] | None = None) -> None:
        self._emit("UNVERIFIED", check, msg, details)

    def err(self, check: str, msg: str, details: list[str] | None = None) -> None:
        self._emit("ERROR", check, msg, details)

    def to_dict(self, repo_root: Path) -> dict[str, Any]:
        return {
            "validator_version": VALIDATOR_VERSION,
            "timestamp_utc": datetime.now(timezone.utc).isoformat(),
            "repo_root": str(repo_root),
            "python_version": platform.python_version(),
            "summary": {
                "INFO": len(self.infos),
                "WARNING": len(self.warnings),
                "UNVERIFIED": len(self.unverified),
                "ERROR": len(self.errors),
                "exit_code": 1 if self.errors else 0,
            },
            "metrics": self.metrics,
            "findings": [
                {
                    "severity": f.severity,
                    "check": f.check,
                    "message": f.message,
                    "details": f.details,
                }
                for f in self.findings
            ],
        }


def iter_repo_files(repo_root: Path, suffix: str) -> list[Path]:
    """Fast, vendor-pruned recursive file enumerator."""
    out: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(repo_root):
        dirnames[:] = sorted(d for d in dirnames if d not in EXCLUDED_DIR_NAMES)
        for fn in sorted(filenames):
            if fn.endswith(suffix):
                out.append(Path(dirpath) / fn)
    return out


# =============================================================================
# 1. Structure & Raw Control Character Audit
# =============================================================================
def check_structure(repo_root: Path, rep: ValidationReport) -> None:
    missing_dirs = [d for d in REQUIRED_DIRS if not (repo_root / d).is_dir()]
    if missing_dirs:
        rep.err("structure", f"Missing {len(missing_dirs)} required directories", missing_dirs)
    else:
        rep.info("structure", f"All {len(REQUIRED_DIRS)} required directories present.")

    missing_support = [f for f in REQUIRED_ROOT_AND_SUPPORT_FILES if not (repo_root / f).is_file()]
    if missing_support:
        rep.err("structure", f"Missing {len(missing_support)} onboarding/governance/tooling files", missing_support)
    else:
        rep.info("structure", f"All {len(REQUIRED_ROOT_AND_SUPPORT_FILES)} onboarding/governance/tooling files present.")

    missing_audits = [f for f in REQUIRED_AUDIT_FILES if not (repo_root / f).is_file()]
    if missing_audits:
        rep.err("structure", f"Missing {len(missing_audits)} mandatory AUDIT files", missing_audits)
    else:
        rep.info("structure", f"All {len(REQUIRED_AUDIT_FILES)} mandatory AUDIT files present.")

    if (repo_root / "scratch").exists():
        rep.err("structure", "Stale 'scratch/' directory still exists inside repository root.")
    else:
        rep.info("structure", "Repository hygiene clean: no stale scratch/ directory.")


def check_control_chars(repo_root: Path, rep: ValidationReport) -> None:
    files = iter_repo_files(repo_root, ".md") + iter_repo_files(repo_root, ".mmd")
    bad_files: list[str] = []
    for path in files:
        data = path.read_bytes()
        bad_bytes = [b for b in data if b < 0x20 and b not in (0x09, 0x0A, 0x0D)]
        if bad_bytes:
            bad_files.append(f"{path.relative_to(repo_root)}: {len(bad_bytes)} control byte(s)")
    if bad_files:
        rep.err("control_chars", f"Raw ASCII control bytes detected in {len(bad_files)} files", bad_files)
    else:
        rep.info("control_chars", f"Zero raw ASCII control characters across {len(files)} Markdown/Mermaid files.")


# =============================================================================
# 2. Real CMSIS Header Parser & Markdown Register Map Cross-Verification
# =============================================================================
def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def parse_cmsis_base_macros(text: str) -> dict[str, int]:
    """Iteratively evaluate CMSIS #define base address expressions directly from headers."""
    clean = strip_c_comments(text)
    raw: dict[str, str] = {}
    for m in re.finditer(r"^\s*#define\s+([A-Za-z_]\w*)\s+([^\n]+)", clean, re.M):
        name, expr = m.group(1), m.group(2).strip()
        if name.endswith(("_Pos", "_Msk")):
            continue
        if any(k in name for k in ("BASE", "PMAADDR", "SRAMCAN", "PERIPH_BASE")):
            raw[name] = expr

    resolved: dict[str, int] = {}
    for _ in range(16):
        changed = False
        for name, expr in raw.items():
            if name in resolved:
                continue
            e = re.sub(r"\b(0[xX][0-9A-Fa-f]+|\d+)[uUlL]+\b", r"\1", expr)
            tokens = re.findall(r"\b[A-Za-z_]\w*\b", e)
            if all(t in resolved for t in tokens):
                e_sub = re.sub(r"\b[A-Za-z_]\w*\b", lambda mm: str(resolved[mm.group(0)]), e)
                if re.fullmatch(r"[0-9a-fA-FxX\s\+\-\(\)\*\/<>|&]+", e_sub):
                    try:
                        resolved[name] = int(eval(e_sub, {"__builtins__": None}, {}))
                        changed = True
                    except Exception:
                        pass
        if not changed:
            break
    return resolved


def parse_cmsis_structs(text: str) -> dict[str, dict[str, int]]:
    """Compute exact C byte offsets for every typedef struct in CMSIS headers."""
    clean = strip_c_comments(text)
    structs: dict[str, dict[str, int]] = {}
    for m in re.finditer(r"typedef\s+struct\s*\{", clean):
        start = m.end()
        depth = 1
        i = start
        while i < len(clean) and depth > 0:
            if clean[i] == "{":
                depth += 1
            elif clean[i] == "}":
                depth -= 1
            i += 1
        body = clean[start:i - 1]
        tail_m = re.match(r"\s*(\w+)\s*;", clean[i:])
        if not tail_m:
            continue
        sname = tail_m.group(1)
        offsets: dict[str, int] = {}
        cur = 0
        j = 0
        while j < len(body):
            while j < len(body) and body[j].isspace():
                j += 1
            if j >= len(body):
                break
            if body[j] == "#":
                nl = body.find("\n", j)
                j = len(body) if nl == -1 else nl + 1
                continue
            um = re.match(r"union\s*\{", body[j:])
            if um:
                u_start = j + um.end()
                ud = 1
                k = u_start
                while k < len(body) and ud > 0:
                    if body[k] == "{":
                        ud += 1
                    elif body[k] == "}":
                        ud -= 1
                    k += 1
                u_body = body[u_start:k - 1]
                max_sz = 4
                for fm in re.finditer(
                    r"\b(uint32_t|uint16_t|uint8_t|int32_t|int16_t|int8_t)\s+(\w+)(?:\[(\d+)\])?\s*;",
                    u_body,
                ):
                    sz, _ = C_TYPE_SIZES[fm.group(1)]
                    cnt = int(fm.group(3)) if fm.group(3) else 1
                    max_sz = max(max_sz, sz * cnt)
                if cur % 4 != 0:
                    cur = (cur + 3) & ~3
                u_tail = re.match(r"\s*(\w+)?\s*;", body[k:])
                if u_tail and u_tail.group(1):
                    offsets[u_tail.group(1)] = cur
                cur += max_sz
                j = k + (u_tail.end() if u_tail else 1)
                continue

            semi = body.find(";", j)
            if semi == -1:
                break
            stmt = body[j:semi].strip()
            j = semi + 1
            stmt_clean = re.sub(r"\b(__IOM|__IM|__OM|__IO|__I|__O|volatile|const)\b", "", stmt).strip()
            tm = re.match(r"^(\w+)\s+(.+)$", stmt_clean)
            if not tm:
                continue
            tname, rest = tm.group(1), tm.group(2)
            if tname not in C_TYPE_SIZES:
                continue
            sz, align = C_TYPE_SIZES[tname]
            for decl in rest.split(","):
                dm = re.match(r"^\s*(\w+)(?:\s*\[\s*(0x[0-9A-Fa-f]+|\d+)[uUlL]*\s*\])?\s*$", decl.strip())
                if not dm:
                    continue
                fname = dm.group(1)
                cnt = int(dm.group(2), 0) if dm.group(2) else 1
                if cur % align != 0:
                    cur = (cur + align - 1) & ~(align - 1)
                offsets[fname] = cur
                cur += sz * cnt
        structs[sname] = offsets
    return structs


def parse_cmsis_bitfields_and_irqs(text: str) -> tuple[dict[str, int], dict[str, int], dict[str, int]]:
    """Extract _Pos, _Msk, and IRQn_Type enum values from CMSIS headers."""
    clean = strip_c_comments(text)
    pos_map: dict[str, int] = {}
    msk_map: dict[str, int] = {}
    irq_map: dict[str, int] = {}

    for m in re.finditer(r"^\s*#define\s+([A-Za-z_]\w*_Pos)\s+\(?(\d+)[uUlL]*\)?", clean, re.M):
        pos_map[m.group(1)] = int(m.group(2))
    for m in re.finditer(
        r"^\s*#define\s+([A-Za-z_]\w*_Msk)\s+\(?(0[xX][0-9A-Fa-f]+)[uUlL]*\s*<<\s*([A-Za-z_]\w*_Pos)\)?",
        clean,
        re.M,
    ):
        p = pos_map.get(m.group(3))
        if p is not None:
            msk_map[m.group(1)] = int(m.group(2), 16) << p

    enum_m = re.search(r"typedef\s+enum\s*\{(.*?)\}\s*IRQn_Type\s*;", clean, re.S)
    if enum_m:
        for em in re.finditer(r"(\w+_IRQn)\s*=\s*(-?\d+)", enum_m.group(1)):
            irq_map[em.group(1)] = int(em.group(2))
    return pos_map, msk_map, irq_map


def check_cmsis_register_cross_check(repo_root: Path, rep: ValidationReport) -> None:
    hdr_g4 = repo_root / "23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h"
    hdr_cm4 = repo_root / "23-code-library/Drivers/CMSIS/Core/Include/core_cm4.h"
    if not hdr_g4.is_file() or not hdr_cm4.is_file():
        rep.err("cmsis_registers", f"CMSIS headers missing: {hdr_g4.exists()=}, {hdr_cm4.exists()=}")
        return

    txt_g4 = hdr_g4.read_text(encoding="utf-8", errors="replace")
    txt_cm4 = hdr_cm4.read_text(encoding="utf-8", errors="replace")
    combined = txt_cm4 + "\n" + txt_g4

    bases = parse_cmsis_base_macros(combined)
    structs = parse_cmsis_structs(combined)
    pos_map, msk_map, irq_map = parse_cmsis_bitfields_and_irqs(combined)

    missing_req_bases = [b for b in REQUIRED_CMSIS_BASES if b not in bases]
    if missing_req_bases:
        rep.err("cmsis_registers", f"Missing required CMSIS base macros in stm32g474xx.h", missing_req_bases)
        return

    if irq_map.get("FMAC_IRQn") != 101:
        rep.err("cmsis_registers", f"stm32g474xx.h IRQn_Type check failed: FMAC_IRQn={irq_map.get('FMAC_IRQn')} (expected 101)")
    else:
        rep.info(
            "cmsis_registers",
            f"Parsed CMSIS headers (stm32g474xx.h + core_cm4.h): {len(bases)} base macros, "
            f"{len(structs)} typedef structs, {len(pos_map)} _Pos / {len(msk_map)} _Msk bitfields, "
            f"{len(irq_map)} IRQns (max FMAC_IRQn=101).",
        )

    # Map peripheral token -> base address
    periph_bases: dict[str, int] = {}
    for k, v in bases.items():
        if k.endswith("_BASE"):
            periph_bases[k[:-5]] = v
    periph_bases["FLASH"] = bases["FLASH_R_BASE"]
    if "USB_PMAADDR" in bases:
        periph_bases["USB_PMAADDR"] = bases["USB_PMAADDR"]
    if "SRAMCAN_BASE" in bases:
        periph_bases["SRAMCAN_BASE"] = bases["SRAMCAN_BASE"]

    reg_dir = repo_root / "20-technical-diagrams" / "registers"
    reg_files = sorted(reg_dir.glob("*.md")) if reg_dir.is_dir() else []
    if len(reg_files) < 16:
        rep.err("cmsis_registers", f"Expected >= 16 peripheral register maps, found {len(reg_files)}.")
        return

    checked_bases = 0
    checked_offsets = 0
    checked_bits = 0
    mismatches: list[str] = []
    missing_citations: list[str] = []
    official_ref_re = re.compile(r"\b(?:RM0440|PM0214|ARMv7-M)\b")

    for md in reg_files:
        txt = md.read_text(encoding="utf-8", errors="replace")
        lines = txt.splitlines()

        if not official_ref_re.search(txt):
            missing_citations.append(f"{md.name}: missing official reference citation (RM0440 or PM0214/ARMv7-M)")

        # Pass A: Cross-verify peripheral base address declarations
        for lineno, ln in enumerate(lines, 1):
            # Message RAM sub-partition line documents SRAMCAN_BASE partitions, not FDCAN register base
            is_msg_ram_line = ("SRAMCAN_BASE" in ln or "Message RAM" in ln)
            for m in re.finditer(r"`([A-Za-z0-9_]+)`\s*(?::|\(|@|—)?\s*`(0x[0-9A-Fa-f]{8})`", ln):
                name, addr_s = m.group(1), m.group(2)
                if is_msg_ram_line and name in ("FDCAN1", "FDCAN2", "FDCAN3"):
                    continue
                if name in periph_bases:
                    checked_bases += 1
                    addr = int(addr_s, 16)
                    if periph_bases[name] != addr:
                        mismatches.append(
                            f"BASE MISMATCH {md.name}:{lineno} {name}: doc={addr_s} vs CMSIS=0x{periph_bases[name]:08X}"
                        )
            for m in re.finditer(r"`(0x[0-9A-Fa-f]{8})`\s*\(`([A-Za-z0-9_]+)`\)", ln):
                addr_s, name = m.group(1), m.group(2)
                if is_msg_ram_line and name in ("FDCAN1", "FDCAN2", "FDCAN3"):
                    continue
                if name in periph_bases:
                    checked_bases += 1
                    addr = int(addr_s, 16)
                    if periph_bases[name] != addr:
                        mismatches.append(
                            f"BASE MISMATCH {md.name}:{lineno} {name}: doc={addr_s} vs CMSIS=0x{periph_bases[name]:08X}"
                        )
            for m in re.finditer(
                r"\*\*([A-Za-z0-9_]+)\s+(?:Interface\s+)?Base(?:\s+Address)?:\*\*\s*`(0x[0-9A-Fa-f]{8})`",
                ln,
            ):
                name, addr_s = m.group(1), m.group(2)
                if name in periph_bases:
                    checked_bases += 1
                    addr = int(addr_s, 16)
                    if periph_bases[name] != addr:
                        mismatches.append(
                            f"BASE MISMATCH {md.name}:{lineno} {name}: doc={addr_s} vs CMSIS=0x{periph_bases[name]:08X}"
                        )

        # Pass B: Cross-verify Markdown register overview tables against C typedef struct offsets
        active_structs: list[str] = []
        in_reg_table = False
        is_abs_addr_table = False

        for lineno, ln in enumerate(lines, 1):
            s = ln.strip()
            if s.startswith("#"):
                in_reg_table = False
                is_abs_addr_table = False
                found_types = re.findall(r"\b([A-Za-z0-9_]+(?:TypeDef|_Type))\b", s)
                valid_types = [t for t in found_types if t in structs]
                if valid_types:
                    active_structs = valid_types
                elif "USB" in s and "CRS" in s:
                    active_structs = ["USB_TypeDef", "CRS_TypeDef"]
                elif "DMAMUX1" in s:
                    active_structs = [
                        "DMAMUX_Channel_TypeDef",
                        "DMAMUX_ChannelStatus_TypeDef",
                        "DMAMUX_RequestGen_TypeDef",
                        "DMAMUX_RequestGenStatus_TypeDef",
                    ]
                elif "DWT" in s and ("ITM" in s or "SWO" in s or "Trace" in s):
                    active_structs = ["DWT_Type", "TPI_Type", "ITM_Type", "CoreDebug_Type"]
                continue

            if not s.startswith("|"):
                in_reg_table = False
                continue

            cells = [c.strip() for c in s.strip("|").split("|")]
            if len(cells) < 2:
                continue
            c0_low, c1_low = cells[0].lower(), cells[1].lower()
            if ("register" in c0_low) and ("offset" in c1_low or "address" in c1_low):
                in_reg_table = True
                is_abs_addr_table = ("address" in c1_low) and ("offset" not in c1_low)
                continue
            if in_reg_table and set(cells[0].replace("-", "").replace(":", "").strip()) == set():
                continue
            if not in_reg_table or not active_structs:
                continue

            raw_reg = re.sub(r"[*`]", "", cells[0]).strip()
            raw_off = re.sub(r"[*`]", "", cells[1]).strip()

            if "*(x" in raw_off or "x-1" in raw_off:
                continue

            off_hexes = re.findall(r"\+?(0x[0-9A-Fa-f]+)", raw_off)
            if not off_hexes:
                continue
            is_range = ".." in raw_off

            paren_member = re.search(r"\(([A-Za-z0-9_]+)\[(\d+)\]\)", raw_reg)
            pairs: list[tuple[str, int]] = []
            if paren_member and len(off_hexes) == 1:
                pairs.append((f"{paren_member.group(1)}[{paren_member.group(2)}]", int(off_hexes[0], 16)))
            elif is_range:
                if "->" in raw_reg:
                    lhs, rhs = raw_reg.split("->", 1)
                    lhs_base = lhs.split("..")[0].strip()
                    pairs.append((f"{lhs_base}->{rhs.split()[0]}", int(off_hexes[0], 16)))
                else:
                    m_rng = re.match(r"([A-Za-z0-9_]+?)\.\.", raw_reg)
                    if m_rng:
                        pairs.append((m_rng.group(1), int(off_hexes[0], 16)))
            else:
                reg_parts = [p.strip() for p in raw_reg.split("/")]
                if len(reg_parts) == len(off_hexes):
                    base_prefix = ""
                    prev_full = ""
                    for idx, (rp, oh) in enumerate(zip(reg_parts, off_hexes)):
                        rp_tok = rp.split()[0]
                        if idx == 0:
                            prev_full = rp_tok
                            if "_" in rp_tok:
                                base_prefix = rp_tok.split("_", 1)[0] + "_"
                            pairs.append((rp_tok, int(oh, 16)))
                        else:
                            # Handle shorthand like I2C_OAR1/2 or FLASH_PCROP1SR/ER or FLASH_WRP1AR/BR
                            if (
                                "_" not in rp_tok
                                and "->" not in rp_tok
                                and prev_full
                                and (rp_tok.isdigit() or (len(rp_tok) == 2 and rp_tok in ("ER", "BR")))
                            ):
                                expanded = prev_full[:-len(rp_tok)] + rp_tok
                                pairs.append((expanded, int(oh, 16)))
                            elif "_" not in rp_tok and "->" not in rp_tok and base_prefix:
                                pairs.append((base_prefix + rp_tok, int(oh, 16)))
                            else:
                                pairs.append((rp_tok, int(oh, 16)))
                elif len(off_hexes) == 1:
                    pairs.append((raw_reg.split()[0], int(off_hexes[0], 16)))

            for rtok, doc_val in pairs:
                if is_abs_addr_table and "->" in rtok:
                    p_name, m_name = rtok.split("->", 1)
                    m_name = re.sub(r"\[.*$", "", m_name)
                    p_base = periph_bases.get(p_name)
                    s_name = f"{p_name}_Type"
                    if p_base is not None and s_name in structs and m_name in structs[s_name]:
                        expected = p_base + structs[s_name][m_name]
                        checked_offsets += 1
                        if expected != doc_val:
                            mismatches.append(
                                f"ABS ADDR MISMATCH {md.name}:{lineno} {rtok}: doc=0x{doc_val:08X} vs CMSIS=0x{expected:08X}"
                            )
                    continue

                member = rtok
                arr_idx = 0
                am = re.match(r"^(.+?)\[(\d+)", member)
                if am:
                    member = am.group(1)
                    arr_idx = int(am.group(2))

                if "->" in member:
                    lhs, member = member.split("->", 1)
                    if lhs == "FDCAN_CONFIG" and "FDCAN_Config_TypeDef" in structs:
                        s_cand = ["FDCAN_Config_TypeDef"]
                    elif "ChannelStatus" in lhs:
                        s_cand = ["DMAMUX_ChannelStatus_TypeDef"]
                    elif "RequestGenStatus" in lhs:
                        s_cand = ["DMAMUX_RequestGenStatus_TypeDef"]
                    elif "RequestGen" in lhs:
                        s_cand = ["DMAMUX_RequestGen_TypeDef"]
                    elif "Channel" in lhs:
                        s_cand = ["DMAMUX_Channel_TypeDef"]
                    else:
                        s_cand = active_structs
                else:
                    s_cand = active_structs
                    if "_" in member:
                        _, rest = member.split("_", 1)
                        if not any(member in structs[s] for s in s_cand):
                            member = rest

                for s_name in s_cand:
                    sm = structs[s_name]
                    if member in sm:
                        expected = sm[member] + arr_idx * 4
                        if s_name == "DMAMUX_ChannelStatus_TypeDef" and doc_val >= 0x80:
                            expected += 0x80
                        elif s_name == "DMAMUX_RequestGen_TypeDef" and doc_val >= 0x100:
                            expected += 0x100
                        elif s_name == "DMAMUX_RequestGenStatus_TypeDef" and doc_val >= 0x140:
                            expected += 0x140
                        checked_offsets += 1
                        if expected != doc_val:
                            mismatches.append(
                                f"OFFSET MISMATCH {md.name}:{lineno} {s_name}.{member} (from '{rtok}'): "
                                f"doc=0x{doc_val:03X} vs CMSIS=0x{expected:03X}"
                            )
                        break

        # Pass C: Cross-verify explicit bit-position claims in bitfield sections against CMSIS *_Pos
        cur_reg_prefix: str | None = None
        for lineno, ln in enumerate(lines, 1):
            hm = re.match(r"^#{2,4}\s+.*?`([A-Z][A-Z0-9]+_[A-Z0-9]+)`", ln)
            if hm:
                cur_reg_prefix = hm.group(1)
                continue
            # Also match bullet starting with **`REG_NAME` Bit N (`FIELD`):**
            bm_inline = re.search(
                r"\*\*`([A-Z][A-Z0-9]+_[A-Z0-9]+)`\s+Bit\s+(\d+)\s+\(`([A-Za-z0-9_]+)`\)",
                ln,
            )
            if bm_inline:
                reg_p, bit_n, fld = bm_inline.group(1), int(bm_inline.group(2)), bm_inline.group(3)
                macro = f"{reg_p}_{fld}_Pos"
                if macro in pos_map:
                    checked_bits += 1
                    if pos_map[macro] != bit_n:
                        mismatches.append(
                            f"BITPOS MISMATCH {md.name}:{lineno} {macro}: doc=bit {bit_n} vs CMSIS=bit {pos_map[macro]}"
                        )
                continue
            if cur_reg_prefix:
                bm = re.search(r"\*\*Bit\s+(\d+)\s+\(`([A-Za-z0-9_]+)`\)", ln)
                if bm:
                    bit_n, fld = int(bm.group(1)), bm.group(2)
                    macro = f"{cur_reg_prefix}_{fld}_Pos"
                    if macro in pos_map:
                        checked_bits += 1
                        if pos_map[macro] != bit_n:
                            mismatches.append(
                                f"BITPOS MISMATCH {md.name}:{lineno} {macro}: doc=bit {bit_n} vs CMSIS=bit {pos_map[macro]}"
                            )

        # Pass C2 (v4.1.0 — Blind Spot M2): Cross-verify table-cell FIELD[hi:lo] & (bN) bitfields against CMSIS _Pos/_Msk
        for lineno, ln in enumerate(lines, 1):
            s = ln.strip()
            if not s.startswith("|"):
                continue
            cells = [c.strip() for c in s.strip("|").split("|")]
            if len(cells) < 3:
                continue
            raw_reg = re.sub(r"[*`]", "", cells[0]).strip().split()[0]
            if not raw_reg or raw_reg.lower() in ("register", "bit", "field") or set(raw_reg) <= {"-", ":"}:
                continue
            reg_clean = re.sub(r"\[.*$", "", raw_reg.split("/")[0])
            cands = [reg_clean]
            if "x_" in reg_clean:
                cands.append(reg_clean.replace("x_", "_"))
            if re.search(r"[0-9]+_", reg_clean):
                cands.append(re.sub(r"[0-9]+_", "_", reg_clean))
            desc = " | ".join(cells[2:])

            # Pattern 1: FIELD[hi:lo]
            for bm in re.finditer(r"`?([A-Z][A-Z0-9_]{1,20})\[(\d+):(\d+)\]`?", desc):
                fld, hi, lo = bm.group(1), int(bm.group(2)), int(bm.group(3))
                if hi < lo or hi > 31:
                    continue
                for rp in cands:
                    pk = f"{rp}_{fld}_Pos"
                    mk = f"{rp}_{fld}_Msk"
                    if pk in pos_map and mk in msk_map:
                        c_lo = pos_map[pk]
                        raw_m = msk_map[mk] >> c_lo
                        # Only validate contiguous bitfields (skip split masks like TIM_CR2_MMS bit25+bits[6:4])
                        if raw_m == 0 or (raw_m & (raw_m + 1)) != 0:
                            break
                        c_width = raw_m.bit_length()
                        c_hi = c_lo + c_width - 1
                        checked_bits += 1
                        # ST RM0440 tables sometimes use field-relative width notation FIELD[w-1:0] when c_lo > 0
                        if lo == 0 and c_lo > 0:
                            if (hi + 1) != c_width:
                                mismatches.append(
                                    f"TABLE BITWIDTH MISMATCH {md.name}:{lineno} {rp}_{fld}: "
                                    f"doc=[{hi}:{lo}] (width {hi + 1}) vs CMSIS=[{c_hi}:{c_lo}] (width {c_width})"
                                )
                        elif (hi, lo) != (c_hi, c_lo):
                            mismatches.append(
                                f"TABLE BITFIELD MISMATCH {md.name}:{lineno} {rp}_{fld}: "
                                f"doc=[{hi}:{lo}] vs CMSIS=[{c_hi}:{c_lo}]"
                            )
                        break

            # Pattern 2: `FIELD` (bN) or `FIELD` (bit N)
            for bm in re.finditer(r"`([A-Z][A-Z0-9_]{1,20})`\s*\((?:b|bit\s+)(\d+)\)", desc):
                fld, bit_n = bm.group(1), int(bm.group(2))
                if bit_n > 31:
                    continue
                for rp in cands:
                    pk = f"{rp}_{fld}_Pos"
                    mk = f"{rp}_{fld}_Msk"
                    if pk in pos_map and mk in msk_map:
                        c_lo = pos_map[pk]
                        raw_m = msk_map[mk] >> c_lo
                        if raw_m == 0 or (raw_m & (raw_m + 1)) != 0:
                            break
                        c_hi = c_lo + raw_m.bit_length() - 1
                        checked_bits += 1
                        if bit_n != c_lo or c_hi != c_lo:
                            mismatches.append(
                                f"TABLE BITPOS MISMATCH {md.name}:{lineno} {rp}_{fld}: "
                                f"doc=bit {bit_n} vs CMSIS=[{c_hi}:{c_lo}]"
                            )
                        break

    cited_files = len(reg_files) - len(missing_citations)
    scope_note = (
        "scope limited to parsed table entries, bullet bit positions & CMSIS C snippets, "
        "not every RM0440 reset value or unparsed prose field"
    )
    rep.metrics["cmsis_cross_check"] = {
        "register_map_files": len(reg_files),
        "cited_official_reference_files": cited_files,
        "missing_citations": len(missing_citations),
        "verified_bases": checked_bases,
        "verified_offsets": checked_offsets,
        "verified_bit_positions": checked_bits,
        "mismatches": len(mismatches),
        "scope_boundary": scope_note,
    }
    if missing_citations:
        rep.err(
            "cmsis_registers",
            f"Official reference citation check FAILED: {len(missing_citations)}/{len(reg_files)} register map(s) missing RM0440 or PM0214/ARMv7-M citation",
            missing_citations,
        )
    if mismatches:
        rep.err(
            "cmsis_registers",
            f"CMSIS ground-truth cross-check FAILED with {len(mismatches)} mismatch(es) "
            f"(verified {checked_bases} bases, {checked_offsets} struct offsets, {checked_bits} bit positions across {len(reg_files)} maps)",
            mismatches,
        )
    elif checked_bases < 50 or checked_offsets < 250:
        rep.err(
            "cmsis_registers",
            f"CMSIS cross-check coverage suspiciously low: bases={checked_bases}, offsets={checked_offsets}.",
        )
    else:
        rep.info(
            "cmsis_registers",
            f"CMSIS ground-truth cross-check passed ({cited_files}/{len(reg_files)} maps cite RM0440 or PM0214/ARMv7-M): "
            f"verified {checked_bases} bases, {checked_offsets} struct offsets, {checked_bits} bit positions "
            f"across {len(reg_files)} maps — {scope_note} (0 mismatches).",
        )


# =============================================================================
# 3. Evidence-Based COMPLETE Concept Audit & DAG Verification
# =============================================================================
def extract_concept_keywords(title: str) -> list[str]:
    clean = re.sub(r"`|\$|\\[a-zA-Z]+", " ", title)
    tokens = re.findall(r"[A-Za-z0-9_]{3,}", clean)
    kws: list[str] = []
    seen = set()
    for t in tokens:
        tl = t.lower()
        if tl in STOPWORDS or tl.isdigit():
            continue
        if tl not in seen:
            seen.add(tl)
            kws.append(tl)
    return kws


def read_location_corpus(repo_root: Path, loc: str) -> tuple[list[Path], str, int]:
    """Resolve semicolon-separated repository_location entries and return (paths, combined text, max size)."""
    paths: list[Path] = []
    chunks: list[str] = []
    max_size = 0
    for part in loc.split(";"):
        rel = part.split("#")[0].strip()
        if not rel or rel == "N/A":
            continue
        p = repo_root / rel
        if p.is_file():
            paths.append(p)
            sz = p.stat().st_size
            max_size = max(max_size, sz)
            chunks.append(p.read_text(encoding="utf-8", errors="replace"))
        elif p.is_dir():
            for sub in sorted(p.iterdir()):
                if sub.is_file() and sub.suffix in (".md", ".c", ".h", ".mmd", ".csv"):
                    paths.append(sub)
                    sz = sub.stat().st_size
                    max_size = max(max_size, sz)
                    chunks.append(sub.read_text(encoding="utf-8", errors="replace"))
    return paths, "\n".join(chunks), max_size


def check_sot_concepts(repo_root: Path, rep: ValidationReport) -> None:
    sot_path = repo_root / "AUDIT" / "concepts_sot.json"
    if not sot_path.is_file():
        rep.err("sot_concepts", "AUDIT/concepts_sot.json missing.")
        return

    try:
        data = json.loads(sot_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        rep.err("sot_concepts", f"AUDIT/concepts_sot.json invalid JSON: {exc}")
        return

    concepts = data.get("concepts", [])
    if len(concepts) != 350:
        rep.err("sot_concepts", f"Expected 350 concepts in SOT, found {len(concepts)}.")
        return

    ids: dict[str, dict[str, Any]] = {}
    domains: set[Any] = set()
    missing_targets: list[str] = []
    schema_errors: list[str] = []

    required_keys = [
        "id", "concept", "domain_num", "domain", "level",
        "prerequisites", "explanation", "hardware_relevance",
        "practical_implementation", "reference", "learning_resource",
        "completion_criteria", "repository_location",
        "initial_status", "part4_status", "current_status", "hardware_status",
    ]
    for c in concepts:
        cid = c.get("id", "<missing>")
        if cid in ids:
            schema_errors.append(f"Duplicate Concept ID: {cid}")
        ids[cid] = c
        domains.add(c.get("domain_num"))
        for k in required_keys:
            if k not in c or (isinstance(c[k], str) and not c[k].strip()):
                schema_errors.append(f"{cid}: missing/empty field '{k}'")
        for st_key in ("initial_status", "part4_status", "current_status"):
            if c.get(st_key) not in VALID_STATUSES:
                schema_errors.append(f"{cid}: invalid {st_key} '{c.get(st_key)}'")
        if c.get("hardware_status") not in VALID_HW_STATUSES:
            schema_errors.append(f"{cid}: invalid hardware_status '{c.get('hardware_status')}'")
        if c.get("level") not in VALID_LEVELS:
            schema_errors.append(f"{cid}: invalid level '{c.get('level')}'")
        loc = c.get("repository_location", "")
        if loc and loc != "N/A":
            for part in loc.split(";"):
                p = part.split("#")[0].strip()
                if p and not (repo_root / p).exists():
                    missing_targets.append(f"{cid} -> {p}")

    # DAG verification: 0 cycles, 0 unknown prereqs, 0 inverted-level edges
    unknown_prereqs: list[str] = []
    inverted_edges: list[str] = []
    adj: dict[str, list[str]] = defaultdict(list)
    indeg = {cid: 0 for cid in ids}
    for cid, c in ids.items():
        clvl = VALID_LEVELS.get(c.get("level", ""), 0)
        for p in c.get("prerequisites", []):
            if p not in ids:
                unknown_prereqs.append(f"{cid} -> {p}")
            else:
                adj[p].append(cid)
                indeg[cid] += 1
                if VALID_LEVELS.get(ids[p].get("level", ""), 0) > clvl:
                    inverted_edges.append(f"{p} ({ids[p]['level']}) -> {cid} ({c['level']})")

    q = [nid for nid, d in indeg.items() if d == 0]
    visited = 0
    while q:
        u = q.pop()
        visited += 1
        for v in adj[u]:
            indeg[v] -= 1
            if indeg[v] == 0:
                q.append(v)
    cycles = len(ids) - visited

    if schema_errors:
        rep.err("sot_dag", f"SOT schema errors ({len(schema_errors)})", schema_errors)
    elif len(domains) != 27:
        rep.err("sot_dag", f"SOT covers only {len(domains)}/27 required domains.")
    elif missing_targets:
        rep.err("sot_dag", f"SOT points to {len(missing_targets)} non-existent repository locations", missing_targets)
    elif unknown_prereqs or inverted_edges or cycles:
        rep.err(
            "sot_dag",
            f"SOT DAG integrity failed: cycles={cycles}, unknown_prereqs={len(unknown_prereqs)}, "
            f"inverted_edges={len(inverted_edges)}",
            unknown_prereqs[:5] + inverted_edges[:5],
        )
    else:
        rep.info(
            "sot_dag",
            f"Source-of-Truth v{data.get('schema_version')} DAG verified: {len(concepts)} unique IDs, "
            f"{len(domains)}/27 domains, 0 cycles, 0 unknown prereqs, 0 inverted-level edges.",
        )

    # Evidence-based audit for EVERY concept marked current_status == "COMPLETE"
    # Policy: NEVER auto-promote PARTIAL -> COMPLETE; queue ambiguous COMPLETE records for manual review.
    complete_concepts = [c for c in concepts if c.get("current_status") == "COMPLETE"]
    partial_concepts = [c for c in concepts if c.get("current_status") == "PARTIAL"]
    field_errors: list[str] = []
    evidence_errors: list[str] = []
    hw_contradictions: list[str] = []
    hw_pending_ids: list[str] = []
    no_explicit_impl_path_ids: list[str] = []
    manual_review_queue: list[str] = []

    explicit_impl_path_re = re.compile(
        r"(?:18-practical-laboratory/|19-projects/|23-code-library/|tests/|Drivers/|"
        r"\blab-\d+|\bLabs?\s+\d+|\bCapstone\s+\d+|\btest_\w+\.c|\b\w+_g4\.[ch]|"
        r"\bring_buffer\.[ch]|\bfault_handler\.[ch]|\bstartup_stm32g474\w*\.c|"
        r"\bstm32g474\w*\.ld|\bFreeRTOS)",
        re.I,
    )

    # Cache file text reads for speed
    file_cache: dict[str, str] = {}
    for c in complete_concepts:
        cid = c["id"]
        # 1. Substantive metadata fields (>= 20 chars, no placeholders, no pipe-split truncation)
        for fname in ("completion_criteria", "reference", "practical_implementation"):
            val = str(c.get(fname, "")).strip()
            val_up = val.upper()
            if val_up in PLACEHOLDER_STRINGS or any(
                re.search(rf"\b{ph}\b", val_up) for ph in ("TBD", "TODO", "FIXME")
            ):
                field_errors.append(f"{cid}: {fname} contains placeholder ({val!r})")
            elif val.startswith("="):
                field_errors.append(f"{cid}: {fname} appears pipe-truncated ({val!r})")
            elif len(val) < 20:
                field_errors.append(f"{cid}: {fname} too short ({len(val)} < 20 chars): {val!r}")

        # 2. Substantive repository_location evidence
        loc = str(c.get("repository_location", "")).strip()
        if not loc or loc == "N/A":
            evidence_errors.append(f"{cid}: COMPLETE concept has empty/N/A repository_location")
        else:
            combined_chunks: list[str] = []
            max_sz = 0
            for part in loc.split(";"):
                rel = part.split("#")[0].strip()
                if not rel:
                    continue
                p = repo_root / rel
                if p.is_file():
                    max_sz = max(max_sz, p.stat().st_size)
                    if rel not in file_cache:
                        file_cache[rel] = p.read_text(encoding="utf-8", errors="replace")
                    combined_chunks.append(file_cache[rel])
                elif p.is_dir():
                    for sub in sorted(p.iterdir()):
                        if sub.is_file() and sub.suffix in (".md", ".c", ".h"):
                            sub_rel = str(sub.relative_to(repo_root))
                            max_sz = max(max_sz, sub.stat().st_size)
                            if sub_rel not in file_cache:
                                file_cache[sub_rel] = sub.read_text(encoding="utf-8", errors="replace")
                            combined_chunks.append(file_cache[sub_rel])
            corpus = "\n".join(combined_chunks)
            if not corpus or max_sz < 400:
                evidence_errors.append(
                    f"{cid}: repository_location '{loc}' is missing or a tiny stub (max_size={max_sz} B)"
                )
            else:
                has_id = cid in corpus
                if not has_id:
                    kws = extract_concept_keywords(str(c.get("concept", "")))
                    corpus_low = corpus.lower()
                    matched_kws = [kw for kw in kws if kw in corpus_low]
                    needed = min(2, len(kws)) if kws else 1
                    if len(matched_kws) < needed:
                        evidence_errors.append(
                            f"{cid}: repository_location '{loc}' mentions neither ID '{cid}' "
                            f"nor >= {needed} title keywords (matched {matched_kws} of {kws[:6]})"
                        )

        # 3. Hardware status honesty & Manual-Review Queue (v4.2.0 — Section 4.B)
        hw = c.get("hardware_status", "")
        impl_raw = str(c.get("practical_implementation", ""))
        impl_low = impl_raw.lower()
        review_reasons: list[str] = []

        if hw in ("EXTERNAL_HARDWARE_PENDING", "NEEDS_BENCH_INSTRUMENT"):
            # Contradiction if implementation claims live physical board test without pending caveat
            has_pending_caveat = bool(re.search(r"pending|untested|not\s+tested|external.*required", impl_low))
            claims_live = bool(
                re.search(r"verified\s+live\s+on|physical\s+nucleo.*pass|onboard\s+hardware\s+tested", impl_low)
            )
            if claims_live and not has_pending_caveat:
                hw_contradictions.append(
                    f"{cid}: hardware_status={hw} contradicts practical_implementation claiming live hardware test"
                )
            else:
                hw_pending_ids.append(cid)
                review_reasons.append(f"hardware_status={hw}")

        if not explicit_impl_path_re.search(impl_raw):
            no_explicit_impl_path_ids.append(cid)
            review_reasons.append("practical_implementation lacks explicit lab/driver/test path")

        if review_reasons:
            manual_review_queue.append(f"{cid}: {'; '.join(review_reasons)}")

    rep.metrics["sot_complete_audit"] = {
        "complete_concepts": len(complete_concepts),
        "partial_concepts": len(partial_concepts),
        "field_errors": len(field_errors),
        "evidence_errors": len(evidence_errors),
        "hw_contradictions": len(hw_contradictions),
        "hw_pending_warnings": len(hw_pending_ids),
        "lacks_explicit_impl_path": len(no_explicit_impl_path_ids),
        "manual_review_queue_total": len(manual_review_queue),
        "hw_pending_ids": hw_pending_ids,
        "lacks_explicit_impl_path_ids": no_explicit_impl_path_ids,
    }

    if field_errors:
        rep.err(
            "sot_evidence",
            f"{len(field_errors)} COMPLETE concept(s) have non-substantive/truncated metadata fields (< 20 chars or placeholder)",
            field_errors,
        )
    if evidence_errors:
        rep.err(
            "sot_evidence",
            f"{len(evidence_errors)} COMPLETE concept(s) lack concrete evidence in repository_location",
            evidence_errors,
        )
    if hw_contradictions:
        rep.err(
            "sot_evidence",
            f"{len(hw_contradictions)} COMPLETE concept(s) have contradictory hardware_status vs practical_implementation",
            hw_contradictions,
        )
    if not (field_errors or evidence_errors or hw_contradictions):
        rep.info(
            "sot_evidence",
            f"All {len(complete_concepts)} COMPLETE concepts verified with substantive repository evidence and metadata fields "
            f"({len(partial_concepts)} PARTIAL concepts preserved untouched; never auto-promoted).",
        )
    if manual_review_queue:
        overlap = len(set(hw_pending_ids) & set(no_explicit_impl_path_ids))
        rep.warn(
            "sot_manual_review",
            f"WARNING (MANUAL REVIEW RECOMMENDED): {len(manual_review_queue)} COMPLETE concept(s) flagged for manual review "
            f"({len(hw_pending_ids)} hardware_status in EXTERNAL_HARDWARE_PENDING/NEEDS_BENCH_INSTRUMENT, "
            f"{len(no_explicit_impl_path_ids)} lacking an explicit lab/driver/test path in practical_implementation; "
            f"{overlap} flagged for both).",
            manual_review_queue,
        )


# =============================================================================
# 4. Practical Lab 11-Section + Prerequisites + Clock Profile + Verification Record Audit
# =============================================================================
LAB_REQUIRED_SECTIONS: list[tuple[str, str]] = [
    ("Objective", r"^#{2,3}\s+.*Objective"),
    ("Required Hardware", r"^#{2,3}\s+.*Required\s+Hardware"),
    ("Required Software", r"^#{2,3}\s+.*Required\s+Software"),
    ("Wiring/Pin Config", r"^#{2,3}\s+.*(?:Wiring|Pin\s+Config)"),
    ("Register Config/Calculations", r"^#{2,3}\s+.*(?:Register|Mathematical\s+Derivation|Kernel\s+Config|Architecture\s+&\s+Testability)"),
    ("Code Implementation", r"^#{2,3}\s+.*Code\s+Implementation"),
    ("Expected Output", r"^#{2,3}\s+.*Expected\s+Output"),
    ("Debugging Procedure", r"^#{2,3}\s+.*Debugging\s+Procedure"),
    ("Common Errors/Mistakes", r"^#{2,3}\s+.*(?:Common\s+Errors|Common\s+Mistakes|Troubleshooting)"),
    ("Verification Procedure", r"^#{2,3}\s+.*Verification\s+Procedure"),
    ("Extension Exercises", r"^#{2,3}\s+.*Extension\s+Exercises"),
    ("Clock Profile Declaration", r"^#{2,3}\s+.*Clock(?:[-\s]+Configuration)?[-\s]+Profile"),
    ("Standardized Verification Record", r"^#{2,3}\s+.*(?:Standardized\s+)?Verification\s+Record"),
]

LAB_PREREQUISITES_RE = re.compile(
    r"(?:^#{2,4}\s+.*Prerequisites|\*\*Prerequisites:?\*\*|\*\*Prerequisites:\s)",
    re.M | re.I,
)
LAB_PREREQUISITES_STRICT = True


def check_labs(repo_root: Path, rep: ValidationReport) -> None:
    labs_dir = repo_root / "18-practical-laboratory"
    if not labs_dir.is_dir():
        rep.err("labs", "18-practical-laboratory/ directory missing.")
        return
    labs = sorted(d for d in labs_dir.iterdir() if d.is_dir() and d.name.startswith("lab-"))
    if len(labs) < 20:
        rep.err("labs", f"Expected >= 20 practical labs, found {len(labs)}.")
        return

    lab_errors: list[str] = []
    missing_prereq: list[str] = []
    for lab_dir in labs:
        readme = lab_dir / "README.md"
        if not readme.is_file():
            lab_errors.append(f"{lab_dir.name}: README.md missing")
            continue
        txt = readme.read_text(encoding="utf-8", errors="replace")
        missing = [
            sec_name
            for sec_name, pattern in LAB_REQUIRED_SECTIONS
            if not re.search(pattern, txt, re.M | re.I)
        ]
        if missing:
            lab_errors.append(f"{lab_dir.name}: missing {len(missing)} section(s): {', '.join(missing)}")
        if not LAB_PREREQUISITES_RE.search(txt):
            missing_prereq.append(f"{lab_dir.name}: missing Prerequisites heading or bold field")

    rep.metrics["labs"] = {
        "total": len(labs),
        "failing_core_sections": len(lab_errors),
        "missing_prerequisites": len(missing_prereq),
        "prerequisites_strict_error": LAB_PREREQUISITES_STRICT,
    }
    if lab_errors:
        rep.err("labs", f"{len(lab_errors)}/{len(labs)} practical labs missing mandatory sections", lab_errors)
    else:
        rep.info(
            "labs",
            f"All {len(labs)}/20 practical labs verified with all 11 core sections + "
            "Clock Profile Declaration + Standardized Verification Record.",
        )

    if missing_prereq:
        if LAB_PREREQUISITES_STRICT:
            rep.err(
                "labs_prerequisites",
                f"{len(missing_prereq)}/{len(labs)} practical lab(s) missing explicit Prerequisites declaration",
                missing_prereq,
            )
        else:
            rep.warn(
                "labs_prerequisites",
                f"{len(missing_prereq)}/{len(labs)} practical lab(s) missing explicit Prerequisites declaration",
                missing_prereq,
            )
    else:
        rep.info(
            "labs_prerequisites",
            f"All {len(labs)}/20 practical labs include an explicit Prerequisites heading or bold field.",
        )


# =============================================================================
# 5. Markdown Links, Reference-Style Links, Images, MathJax & External URLs
# =============================================================================
def strip_fenced_blocks_preserving_lines(text: str) -> list[tuple[int, str]]:
    """Return list of (1-based lineno, line) for lines outside fenced code blocks."""
    out: list[tuple[int, str]] = []
    in_fence = False
    fence_tok = ""
    for lineno, line in enumerate(text.splitlines(), 1):
        stripped = line.lstrip()
        if stripped.startswith("```") or stripped.startswith("~~~"):
            tok = stripped[:3]
            if not in_fence:
                in_fence = True
                fence_tok = tok
            elif stripped.startswith(fence_tok):
                in_fence = False
            continue
        if not in_fence:
            out.append((lineno, line))
    return out


def strip_inline_code_with_space(line: str) -> str:
    """Replace inline code spans with spaces so adjacent brackets never merge."""
    return re.sub(r"``[^`]*``|`[^`]*`", " ", line)


def scan_mathjax_in_file(md_path: Path, repo_root: Path) -> list[str]:
    """
    Expanded GitHub MathJax Safety Audit (v4.1.0 — closes Blind Spots M5 & M6):
      1. Unescaped _ or \\_ inside \\text{}, \\mathrm{}, \\texttt{}, \\textbf{}, \\textit{}, \\mbox{}
      2. Bare \\_ anywhere inside $...$ / $$...$$ math spans (GFM strips \\ before MathJax parses!)
      3. Unbalanced { / } braces inside math spans
      4. Unclosed inline $ or display $$ math delimiters & unescaped | inside table math
    """
    text = md_path.read_text(encoding="utf-8", errors="replace")
    rel = str(md_path.relative_to(repo_root))
    violations: list[str] = []

    def check_math_expr(expr: str, lineno: int, in_table_row: bool = False) -> None:
        # 1. Text-mode macros containing _ or \_ (M5)
        for m in re.finditer(r"\\(?:text|mathrm|texttt|textbf|textit|mbox)\{", expr):
            i = m.end()
            depth = 1
            while i < len(expr) and depth > 0:
                if expr[i] == "{" and (i == 0 or expr[i - 1] != "\\"):
                    depth += 1
                elif expr[i] == "}" and (i == 0 or expr[i - 1] != "\\"):
                    depth -= 1
                i += 1
            if depth != 0:
                violations.append(f"{rel}:{lineno}: unclosed brace in {expr[m.start():m.start() + 40]!r}")
            else:
                inner = expr[m.end():i - 1]
                if "_" in inner:
                    snippet = expr[m.start():i]
                    violations.append(f"{rel}:{lineno}: {snippet}")

        # 2. Bare \_ anywhere inside math (M6)
        if r"\_" in expr:
            violations.append(f"{rel}:{lineno}: bare \\_ inside math: {expr[:60]!r}")

        # 3. Unbalanced { } braces inside math (ignoring \{ and \}) (M6)
        depth = 0
        i = 0
        while i < len(expr):
            if expr[i] == "\\" and i + 1 < len(expr):
                i += 2
                continue
            if expr[i] == "{":
                depth += 1
            elif expr[i] == "}":
                depth -= 1
                if depth < 0:
                    violations.append(f"{rel}:{lineno}: unmatched closing '}}' in math: {expr[:60]!r}")
                    break
            i += 1
        if depth > 0:
            violations.append(f"{rel}:{lineno}: unclosed '{{' in math: {expr[:60]!r}")

        # 4. Unescaped table pipe '|' inside inline math on a Markdown table row
        if in_table_row and re.search(r"(?<!\\)\|", expr):
            violations.append(f"{rel}:{lineno}: unescaped '|' inside table math: {expr[:60]!r}")

    in_display = False
    display_buf: list[str] = []
    display_start = 0

    for lineno, line in strip_fenced_blocks_preserving_lines(text):
        if in_display:
            if "$$" in line:
                before, _, _ = line.partition("$$")
                display_buf.append(before)
                check_math_expr("\n".join(display_buf), display_start)
                in_display = False
                display_buf = []
            else:
                display_buf.append(line)
            continue

        is_tbl = line.lstrip().startswith("|")
        cleaned = strip_inline_code_with_space(line)

        # Single-line or opening display math $$...$$
        pos = 0
        while True:
            idx = cleaned.find("$$", pos)
            if idx == -1:
                break
            end_idx = cleaned.find("$$", idx + 2)
            if end_idx != -1:
                check_math_expr(cleaned[idx + 2:end_idx], lineno, in_table_row=is_tbl)
                cleaned = cleaned[:idx] + " " + cleaned[end_idx + 2:]
                pos = idx + 1
            else:
                in_display = True
                display_start = lineno
                display_buf = [cleaned[idx + 2:]]
                cleaned = cleaned[:idx]
                break

        # Inline $...$ math (avoid escaped \$ and pure currency like $10)
        c2 = re.sub(r"\\\$", "  ", cleaned)
        inline_math_re = re.compile(r"(?<!\$)\$(?!\$)(?!\s)([^$\n]+?)(?<!\s)\$(?!\$)")
        for m in inline_math_re.finditer(c2):
            expr = m.group(1)
            if re.match(r"^\d[\d,\.]*\b", expr) and "\\" not in expr:
                continue
            check_math_expr(expr, lineno, in_table_row=is_tbl)

        # Detect unclosed inline $ followed by math-like tokens (M6)
        rem = inline_math_re.sub(" ", c2)
        for um in re.finditer(r"(?<!\$)\$(?!\$)(.+)", rem):
            tail = um.group(1)
            if not re.match(r"^\s*\d[\d,\.]*(?:\s|$|[^A-Za-z_\\{^])", tail):
                if any(ch in tail for ch in ("\\", "_", "^", "{", "}", "=")):
                    violations.append(f"{rel}:{lineno}: unclosed inline math '$': {um.group(0)[:60]!r}")

    if in_display:
        violations.append(f"{rel}:{display_start}: unclosed display math '$$'")

    return violations


def gfm_heading_slug(heading_text: str) -> str:
    """Compute GitHub Flavored Markdown heading anchor slug."""
    s = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", heading_text)
    s = s.replace("`", "")
    s = re.sub(r"<[^>]+>", "", s)
    s = s.strip().lower()
    out: list[str] = []
    for ch in s:
        if ch.isalnum() or ch in (" ", "-", "_"):
            out.append("-" if ch == " " else ch)
    return "".join(out)


def extract_gfm_anchors(md_path: Path) -> set[str]:
    """Extract all valid GFM heading anchor slugs (incl. duplicate -1/-2 suffixes) from a Markdown file."""
    text = md_path.read_text(encoding="utf-8", errors="replace")
    anchors: set[str] = set()
    counts: dict[str, int] = {}
    for _, line in strip_fenced_blocks_preserving_lines(text):
        m = re.match(r"^#{1,6}\s+(.+?)(?:\s+#+\s*)?$", line)
        if m:
            slug = gfm_heading_slug(m.group(1))
            if slug in counts:
                counts[slug] += 1
                anchors.add(f"{slug}-{counts[slug]}")
            else:
                counts[slug] = 0
                anchors.add(slug)
    return anchors


class _TrackingRedirectHandler(urllib.request.HTTPRedirectHandler):
    """Records exact HTTP 3xx redirect hops while following redirects normally."""

    def __init__(self) -> None:
        super().__init__()
        self.redirects: list[tuple[int, str]] = []

    def redirect_request(self, req: Any, fp: Any, code: int, msg: str, headers: Any, newurl: str) -> Any:
        self.redirects.append((int(code), str(newurl)))
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def probe_single_url(url: str, timeout: float = 6.0) -> tuple[str, str, str]:
    """
    Probe one external URL and classify into exact v4.2.0 taxonomy:
      - accessible     : HTTP 2xx with no redirect
      - redirected     : HTTP 3xx -> final URL ending in 2xx
      - bot_blocked    : HTTP 403 / 429
      - broken         : definite HTTP 404 / 410 dead link
      - network_failed : DNS error, connection refused, timeout, SSL/OSError, or 5xx/other
    """
    headers = {
        "User-Agent": f"Mozilla/5.0 (compatible; EmbeddedC-MasterHub-Validator/{VALIDATOR_VERSION}; +https://github.com)",
        "Accept": "text/html,application/xhtml+xml,*/*;q=0.8",
    }
    for method in ("HEAD", "GET"):
        redir_handler = _TrackingRedirectHandler()
        opener = urllib.request.build_opener(redir_handler)
        req = urllib.request.Request(url, headers=headers, method=method)
        try:
            with opener.open(req, timeout=timeout) as resp:
                code = int(resp.getcode() or 200)
                final_url = str(resp.geturl() or url)
                if 200 <= code < 300:
                    if redir_handler.redirects:
                        first_code = redir_handler.redirects[0][0]
                        return (url, "redirected", f"HTTP {first_code} -> {code} ({final_url})")
                    if final_url != url:
                        return (url, "redirected", f"HTTP 3xx -> {code} ({final_url})")
                    return (url, "accessible", f"HTTP {code}")
                if 300 <= code < 400:
                    return (url, "redirected", f"HTTP {code} -> {final_url}")
                if code in (403, 429):
                    return (url, "bot_blocked", f"HTTP {code}")
                if code in (404, 410):
                    return (url, "broken", f"HTTP {code}")
                return (url, "network_failed", f"HTTP {code}")
        except urllib.error.HTTPError as e:
            if method == "HEAD" and e.code in (403, 405, 501):
                continue
            if e.code in (403, 429):
                return (url, "bot_blocked", f"HTTP {e.code}")
            if e.code in (404, 410):
                return (url, "broken", f"HTTP {e.code}")
            return (url, "network_failed", f"HTTP {e.code}")
        except urllib.error.URLError as e:
            reason = e.reason
            if isinstance(reason, socket.gaierror):
                return (url, "network_failed", f"DNS error: {reason}")
            return (url, "network_failed", f"URLError: {reason}")
        except (TimeoutError, socket.timeout, OSError) as e:
            return (url, "network_failed", f"Timeout/IO: {e}")
    return (url, "network_failed", "Exhausted retries")


def check_markdown_links_and_mathjax(repo_root: Path, rep: ValidationReport) -> set[str]:
    """
    Local-only Markdown validation (Section 4.F):
      - Inline relative links
      - Internal #anchor GFM heading fragments
      - Local image references
      - Reference-style links & definitions
      - Expanded GitHub MathJax safety
    Returns the deduplicated set of external HTTP(S) URLs for cleanly separated probing.
    """
    inline_link_re = re.compile(r"(!?)\[[^\]]*\]\(([^)]+)\)")
    ref_def_re = re.compile(r"^\s*\[([^\]]+)\]:\s*<?(\S+?)>?(?:\s|$)")
    # Negative lookbehind (?<!\w) prevents matching C 2D arrays like buf[32][2]
    ref_use_re = re.compile(r"(?<!\w)(!?)\[([^\[\]\n]+)\]\[([^\[\]\n]*)\]")
    ext_url_re = re.compile(r"https?://[^\s<>)\]`\"']+")

    md_files = iter_repo_files(repo_root, ".md")
    broken_inline: list[str] = []
    broken_images: list[str] = []
    broken_anchors: list[str] = []
    broken_ref_uses: list[str] = []
    broken_ref_targets: list[str] = []
    mathjax_violations: list[str] = []
    ext_urls: set[str] = set()
    anchor_cache: dict[Path, set[str]] = {}

    inline_checked = 0
    images_checked = 0
    anchors_checked = 0
    ref_defs_total = 0
    ref_uses_total = 0

    def verify_anchor(src_md: Path, src_rel: str, lineno: int, raw_target: str, target_path: Path, frag: str) -> None:
        nonlocal anchors_checked
        if not frag or not target_path.is_file() or target_path.suffix.lower() != ".md":
            return
        anchors_checked += 1
        if target_path not in anchor_cache:
            anchor_cache[target_path] = extract_gfm_anchors(target_path)
        if frag.lower() not in anchor_cache[target_path]:
            broken_anchors.append(f"{src_rel}:{lineno} -> {raw_target} (missing heading '#{frag}')")

    for md in md_files:
        rel = str(md.relative_to(repo_root))
        raw = md.read_text(encoding="utf-8", errors="replace")
        non_fenced = strip_fenced_blocks_preserving_lines(raw)

        # 1. Collect reference definitions [id]: target
        ref_defs: dict[str, tuple[int, str]] = {}
        for lineno, line in non_fenced:
            dm = ref_def_re.match(line)
            if dm:
                label = dm.group(1).strip().lower()
                target = dm.group(2).strip()
                ref_defs[label] = (lineno, target)
        ref_defs_total += len(ref_defs)

        # Verify local relative targets & #anchors in reference definitions
        for label, (lineno, raw_target) in ref_defs.items():
            if raw_target.startswith(("http://", "https://")):
                ext_urls.add(raw_target.rstrip(".,;:!?"))
            elif raw_target.startswith("#"):
                verify_anchor(md, rel, lineno, raw_target, md.resolve(), raw_target[1:])
            elif not raw_target.startswith(("mailto:", "file://")):
                t, _, frag = raw_target.partition("#")
                if t:
                    res = (md.parent / t).resolve()
                    if not res.exists():
                        broken_ref_targets.append(f"{rel}:{lineno}: [{label}]: {raw_target}")
                    elif frag:
                        verify_anchor(md, rel, lineno, raw_target, res, frag)

        # 2. Scan inline links, images, #anchors, reference-style links, and collect external URLs
        for lineno, line in non_fenced:
            if ref_def_re.match(line):
                continue
            cleaned = strip_inline_code_with_space(line)

            for m in inline_link_re.finditer(cleaned):
                is_img = bool(m.group(1))
                raw_target = m.group(2).strip()
                if not raw_target:
                    continue
                if raw_target.startswith(("http://", "https://")):
                    ext_urls.add(raw_target.split()[0].rstrip(".,;:!?"))
                    continue
                if raw_target.startswith(("mailto:", "file://")):
                    continue
                first_tok = raw_target.split()[0]
                if first_tok.startswith("#"):
                    if not is_img:
                        verify_anchor(md, rel, lineno, raw_target, md.resolve(), first_tok[1:])
                    continue
                target, _, frag = first_tok.partition("#")
                if not target:
                    continue
                resolved = (md.parent / target).resolve()
                if is_img:
                    images_checked += 1
                    if not resolved.exists():
                        broken_images.append(f"{rel}:{lineno} -> {raw_target}")
                else:
                    inline_checked += 1
                    if not resolved.exists():
                        broken_inline.append(f"{rel}:{lineno} -> {raw_target}")
                    elif frag:
                        verify_anchor(md, rel, lineno, raw_target, resolved, frag)

            for m in ref_use_re.finditer(cleaned):
                text_part = m.group(2).strip()
                id_part = m.group(3).strip()
                # Ignore markdown table / footnote / array numeric indices like [0][1]
                if text_part.isdigit() and id_part.isdigit():
                    continue
                label = (id_part or text_part).lower()
                ref_uses_total += 1
                if label not in ref_defs:
                    broken_ref_uses.append(f"{rel}:{lineno}: [{text_part}][{id_part}] (undefined ref '{label}')")

            for m in ext_url_re.finditer(cleaned):
                ext_urls.add(m.group(0).rstrip(".,;:!?"))

        # 3. GitHub MathJax Safety Audit
        mathjax_violations.extend(scan_mathjax_in_file(md, repo_root))

    # Also collect external URLs from 22-video-library/video-database.csv
    csv_path = repo_root / "22-video-library" / "video-database.csv"
    if csv_path.is_file():
        with csv_path.open("r", encoding="utf-8") as f:
            for row in csv.DictReader(f):
                u = row.get("url", "").strip()
                if u.startswith(("http://", "https://")):
                    ext_urls.add(u)

    rep.metrics["markdown_audit"] = {
        "md_files": len(md_files),
        "inline_links_checked": inline_checked,
        "anchors_checked": anchors_checked,
        "images_checked": images_checked,
        "ref_defs": ref_defs_total,
        "ref_uses": ref_uses_total,
        "broken_inline": len(broken_inline),
        "broken_anchors": len(broken_anchors),
        "broken_images": len(broken_images),
        "broken_ref_links": len(broken_ref_uses) + len(broken_ref_targets),
        "mathjax_violations": len(mathjax_violations),
        "unique_external_urls": len(ext_urls),
    }

    if broken_inline:
        rep.err("links", f"Broken internal inline links ({len(broken_inline)}/{inline_checked})", broken_inline)
    else:
        rep.info("links", f"All {inline_checked} internal inline links verified across {len(md_files)} .md files.")

    if broken_anchors:
        rep.err("anchors", f"Broken internal Markdown #anchor links ({len(broken_anchors)}/{anchors_checked})", broken_anchors)
    else:
        rep.info("anchors", f"All {anchors_checked} internal Markdown #anchor fragments verified against GFM headings (0 broken).")

    if broken_images:
        rep.err("images", f"Broken local image links ({len(broken_images)}/{images_checked})", broken_images)
    else:
        rep.info("images", f"All {images_checked} local image references verified (0 broken).")

    all_broken_refs = broken_ref_uses + broken_ref_targets
    if all_broken_refs:
        rep.err("ref_links", f"Broken reference-style Markdown links ({len(all_broken_refs)})", all_broken_refs)
    else:
        rep.info("ref_links", f"Reference-style links verified: {ref_uses_total} uses, {ref_defs_total} definitions (0 broken).")

    if mathjax_violations:
        rep.err(
            "mathjax",
            f"GitHub MathJax safety FAILED: {len(mathjax_violations)} violation(s) "
            "(text-mode underscores, bare \\_ in math, unbalanced braces, or unclosed $/$$)",
            mathjax_violations,
        )
    else:
        rep.info("mathjax", f"Expanded GitHub MathJax safety passed across all {len(md_files)} .md files (0 violations).")

    return ext_urls


def check_external_urls_live(
    ext_urls: set[str],
    rep: ValidationReport,
    check_external_urls: bool = False,
) -> None:
    """
    External URL probe cleanly separated from local Markdown checks (Section 4.E):
      - Tracks accessible (2xx), redirected (3xx -> final URL), bot_blocked (403/429),
        broken (definite HTTP 404/410), and network/DNS/timeout failures.
      - Offline Resilience Rule: if network connectivity is unavailable (0 HTTP responses),
        emit UNVERIFIED (never ERROR); only emit ERROR on definite HTTP 404/410 when online.
    """
    if not check_external_urls:
        rep.metrics["external_url_probe"] = {"mode": "skipped", "unique_urls": len(ext_urls)}
        rep.unverified_item(
            "external_urls",
            f"{len(ext_urls)} unique external URLs UNVERIFIED (pass --check-external-urls to probe live).",
        )
        return

    sorted_urls = sorted(ext_urls)
    results: dict[str, list[str]] = {
        "accessible": [],
        "redirected": [],
        "bot_blocked": [],
        "broken": [],
        "network_failed": [],
    }
    with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
        futs = [pool.submit(probe_single_url, u) for u in sorted_urls]
        for fut in concurrent.futures.as_completed(futs):
            u, cat, detail = fut.result()
            results[cat].append(f"{u} ({detail})")

    http_responses = (
        len(results["accessible"])
        + len(results["redirected"])
        + len(results["bot_blocked"])
        + len(results["broken"])
    )
    offline = (http_responses == 0) and (len(results["network_failed"]) > 0)

    rep.metrics["external_url_probe"] = {
        "mode": "offline_detected" if offline else "live",
        "unique_urls": len(sorted_urls),
        "accessible_2xx": len(results["accessible"]),
        "redirected_3xx": len(results["redirected"]),
        "bot_blocked_403_429": len(results["bot_blocked"]),
        "broken_404_410": len(results["broken"]),
        "network_dns_timeout_failed": len(results["network_failed"]),
        "redirected_details": sorted(results["redirected"]),
    }

    if offline:
        rep.unverified_item(
            "external_urls",
            f"Offline resilience triggered: network connectivity unavailable "
            f"(0/{len(sorted_urls)} HTTP responses, {len(results['network_failed'])} DNS/connection/timeout failures) "
            "— external URLs UNVERIFIED (not counted as ERROR).",
            sorted(results["network_failed"]),
        )
        return

    rep.info(
        "external_urls",
        f"Live external URL probe ({len(sorted_urls)} URLs): "
        f"{len(results['accessible'])} accessible (2xx), "
        f"{len(results['redirected'])} redirected (3xx -> final URL), "
        f"{len(results['bot_blocked'])} bot-blocked (403/429), "
        f"{len(results['broken'])} broken (404/410), "
        f"{len(results['network_failed'])} network/DNS/timeout failures.",
        sorted(results["redirected"]) if results["redirected"] else None,
    )
    if results["broken"]:
        rep.err(
            "external_urls",
            f"{len(results['broken'])} broken external URL(s) (definite HTTP 404/410)",
            sorted(results["broken"]),
        )
    if results["bot_blocked"]:
        rep.warn(
            "external_urls",
            f"{len(results['bot_blocked'])} bot-blocked external URL(s) (HTTP 403/429)",
            sorted(results["bot_blocked"]),
        )
    if results["network_failed"]:
        rep.unverified_item(
            "external_urls",
            f"{len(results['network_failed'])} external URL(s) failed due to network/DNS/timeout (UNVERIFIED, not ERROR)",
            sorted(results["network_failed"]),
        )


def check_markdown_links_mathjax_urls(
    repo_root: Path,
    rep: ValidationReport,
    check_external_urls: bool = False,
) -> None:
    """Backward-compatible wrapper invoking local Markdown checks then separated external URL checks."""
    ext_urls = check_markdown_links_and_mathjax(repo_root, rep)
    check_external_urls_live(ext_urls, rep, check_external_urls=check_external_urls)


# =============================================================================
# 6. Real Mermaid Diagram Syntax Parser
# =============================================================================
def strip_quoted_strings(line: str) -> str:
    return re.sub(r'"[^"]*"', '""', line)


def check_balanced_delimiters(line: str) -> str | None:
    """Verify balanced [], (), {} outside quoted strings, accounting for Mermaid node shapes & arrows."""
    s = strip_quoted_strings(line)
    # Remove Mermaid arrows/connectors and edge label pipes |...| before bracket balancing
    s = re.sub(r"\|[^|]*\|", " ", s)
    s = re.sub(r"<-->|<==>|-\.->|-\.-|==>|-->>|->>|--x|-x|--\)|-\)|-->|---|===", " ", s)
    # Normalize Mermaid special shapes: ([...]), [(...)], [/.../], [\...\], [/...\], [\.../]
    s = s.replace("([", "( ").replace("])", " )")
    s = s.replace("[(", "[ ").replace(")]", " ]")
    s = s.replace("[/", "[ ").replace("/]", " ]")
    s = s.replace("[\\", "[ ").replace("\\]", " ]")

    stack: list[str] = []
    pairs = {")": "(", "]": "[", "}": "{"}
    for ch in s:
        if ch in "([{":
            stack.append(ch)
        elif ch in ")]}":
            if not stack or stack[-1] != pairs[ch]:
                return f"unbalanced closing '{ch}'"
            stack.pop()
    if stack:
        return f"unclosed '{stack[-1]}'"
    return None


def validate_single_mermaid(mmd_path: Path, repo_root: Path) -> list[str]:
    rel = str(mmd_path.relative_to(repo_root))
    txt = mmd_path.read_text(encoding="utf-8", errors="replace")
    lines = txt.splitlines()
    issues: list[str] = []

    non_empty = [(i, ln.strip()) for i, ln in enumerate(lines, 1) if ln.strip() and not ln.strip().startswith("%%")]
    if not non_empty:
        return [f"{rel}: empty Mermaid diagram"]

    first_lineno, header = non_empty[0]
    if not any(header.startswith(h) for h in VALID_MERMAID_HEADERS):
        issues.append(f"{rel}:{first_lineno}: unsupported Mermaid diagram header '{header}'")

    is_flowchart = header.startswith(("flowchart ", "graph "))
    is_sequence = header.startswith("sequenceDiagram")

    # Block nesting balance: subgraph/end (and sequence loop/alt/opt/par/critical/rect + end)
    block_depth = 0
    block_openers = ("subgraph",) if is_flowchart else ("loop", "alt", "opt", "par", "critical", "rect", "box")
    node_count = 0
    edge_count = 0

    flow_edge_re = re.compile(r"(-->|---|-\.->|-\.-|==>|===|<-->|o--o|x--x|&)")
    seq_msg_re = re.compile(r"(->>|-->>|->|-->|-x|--x|-\)|--\))")
    node_decl_re = re.compile(r"^[A-Za-z_]\w*\s*[\[\(\{]")
    seq_part_re = re.compile(r"^(?:participant|actor)\s+\S+")

    for lineno, line in non_empty[1:]:
        # HTML tag prohibition
        if HTML_TAG_RE.search(line):
            issues.append(f"{rel}:{lineno}: contains prohibited HTML tag in '{line[:80]}'")

        # Balanced double quotes
        if line.count('"') % 2 != 0:
            issues.append(f"{rel}:{lineno}: unbalanced double quotes in '{line[:80]}'")

        # Block opener/closer tracking
        low = line.lower()
        if any(re.match(rf"^{kw}\b", low) for kw in block_openers):
            block_depth += 1
            # Still check balanced brackets/quotes on subgraph header
            delim_err = check_balanced_delimiters(line)
            if delim_err:
                issues.append(f"{rel}:{lineno}: {delim_err} in '{line[:80]}'")
            continue
        if re.match(r"^end\s*$", low):
            block_depth -= 1
            if block_depth < 0:
                issues.append(f"{rel}:{lineno}: unexpected 'end' without matching block opener")
                block_depth = 0
            continue

        # Skip directive lines (style, classDef, class, linkStyle, direction, autonumber, Note)
        if re.match(r"^(?:style|classDef|class|linkStyle|click|direction|autonumber|Note|activate|deactivate|else|and)\b", line, re.I):
            continue

        # Delimiter balance check for flowchart/state/class lines (sequence message payloads are free text after ':')
        if is_sequence:
            if seq_part_re.match(line):
                node_count += 1
            elif seq_msg_re.search(line):
                edge_count += 1
                lhs = line.split(":", 1)[0]
                if not re.search(r"\S+\s*(?:->>|-->>|->|-->|-x|--x|-\)|--\))\s*\S+", lhs):
                    issues.append(f"{rel}:{lineno}: malformed sequence arrow '{lhs}'")
            continue

        # Check unclosed pipe edge labels |...| outside quotes
        no_quotes = strip_quoted_strings(line)
        if no_quotes.count("|") % 2 != 0:
            issues.append(f"{rel}:{lineno}: unbalanced edge-label pipe '|' in '{line[:80]}'")

        # Check invalid single-dash arrow `->` in flowchart/graph outside quotes
        if is_flowchart and re.search(r"(?<![-<.=])->(?![>])", no_quotes):
            issues.append(f"{rel}:{lineno}: invalid flowchart connector '->' (use '-->') in '{line[:80]}'")

        delim_err = check_balanced_delimiters(line)
        if delim_err:
            issues.append(f"{rel}:{lineno}: {delim_err} in '{line[:80]}'")

        if flow_edge_re.search(no_quotes):
            edge_count += 1
            # Count inline node endpoints on edge statements (e.g., A["..."] --> B["..."])
            parts = flow_edge_re.split(no_quotes)
            for part in parts[::2]:
                nm = re.match(r"^\s*(?:\|[^|]*\|\s*)?([A-Za-z_]\w*)", part.strip())
                if nm:
                    node_count += 1
        elif node_decl_re.match(line):
            node_count += 1

    if block_depth != 0:
        issues.append(f"{rel}: unbalanced block structure ({block_depth} unclosed subgraph/block(s) missing 'end')")

    if is_flowchart and (node_count == 0 or edge_count == 0):
        issues.append(f"{rel}: diagram appears trivial/incomplete (nodes={node_count}, edges={edge_count})")
    elif is_sequence and (node_count == 0 or edge_count == 0):
        issues.append(f"{rel}: sequenceDiagram incomplete (participants={node_count}, messages={edge_count})")

    return issues


def check_mermaid_diagrams(repo_root: Path, rep: ValidationReport) -> None:
    """
    Mermaid Validation with Explicit Syntax vs Render Separation (Section 4.D):
      - mermaid_syntax: full Python structural syntax validation (headers, balanced blocks/quotes/
        delimiters, valid connectors, non-zero nodes/edges, 0 HTML tags).
      - mermaid_render: actively probes shutil.which('mmdc') (@mermaid-js/mermaid-cli); renders
        each .mmd to temp SVG if available, or explicitly reports UNVERIFIED when absent.
    """
    mmd_files = iter_repo_files(repo_root, ".mmd")
    if not mmd_files:
        rep.err("mermaid_syntax", "No .mmd Mermaid diagram files found.")
        return

    all_issues: list[str] = []
    for mmd in mmd_files:
        all_issues.extend(validate_single_mermaid(mmd, repo_root))

    syntax_ok = len(all_issues) == 0
    if all_issues:
        rep.err("mermaid_syntax", f"Mermaid static structural syntax validation FAILED with {len(all_issues)} issue(s)", all_issues)
    else:
        rep.info(
            "mermaid_syntax",
            f"All {len(mmd_files)} Mermaid (.mmd) diagrams passed static structural syntax validation.",
        )

    mmdc_exe = shutil.which("mmdc")
    render_status = "UNVERIFIED_TOOLING_UNAVAILABLE"
    rendered_count = 0
    render_failures: list[str] = []

    if not mmdc_exe:
        rep.unverified_item(
            "mermaid_render",
            "Mermaid visual rendering NOT TESTED — 'mmdc' (@mermaid-js/mermaid-cli) tooling unavailable on host.",
        )
    else:
        with tempfile.TemporaryDirectory() as td:
            for idx, mmd in enumerate(mmd_files, 1):
                out_svg = Path(td) / f"diagram_{idx}_{mmd.stem}.svg"
                rel = str(mmd.relative_to(repo_root))
                try:
                    proc = subprocess.run(
                        [mmdc_exe, "-i", str(mmd), "-o", str(out_svg)],
                        capture_output=True,
                        text=True,
                        timeout=30,
                    )
                    if proc.returncode != 0 or not out_svg.is_file() or out_svg.stat().st_size == 0:
                        err_msg = (proc.stderr or proc.stdout).strip().splitlines()
                        tail = err_msg[-1] if err_msg else f"exit {proc.returncode}"
                        render_failures.append(f"{rel}: mmdc render failed ({tail})")
                    else:
                        rendered_count += 1
                except (subprocess.TimeoutExpired, OSError) as exc:
                    render_failures.append(f"{rel}: mmdc render error ({exc})")

        if render_failures:
            render_status = "RENDER_FAILED"
            rep.err(
                "mermaid_render",
                f"Mermaid visual rendering via mmdc FAILED on {len(render_failures)}/{len(mmd_files)} diagram(s)",
                render_failures,
            )
        else:
            render_status = "RENDERED"
            rep.info(
                "mermaid_render",
                f"Successfully rendered all {rendered_count}/{len(mmd_files)} Mermaid (.mmd) diagrams to SVG via mmdc.",
            )

    rep.metrics["mermaid"] = {
        "files": len(mmd_files),
        "syntax_issues": len(all_issues),
        "syntax_validated": syntax_ok,
        "mmdc_available": bool(mmdc_exe),
        "render_status": render_status,
        "rendered_count": rendered_count,
        "render_failures": len(render_failures),
    }


# =============================================================================
# 7. Curriculum 13-Section + References + Nav + Drivers/FreeRTOS/Capstones/Videos
# =============================================================================
CURRICULUM_REQUIRED_SECTIONS: list[tuple[str, str]] = [
    ("1. Objectives", r"^##\s+1\.\s+.*Objectives"),
    ("2. Prerequisites", r"^##\s+2\.\s+.*Prerequisites"),
    ("3. Theory", r"^##\s+3\.\s+.*Theory"),
    ("4. Terminology", r"^##\s+4\.\s+.*Terminology"),
    ("5. Register/Architecture", r"^##\s+5\.\s+.*(?:Register|Architecture)"),
    ("6. Formulas/Calculations", r"^##\s+6\.\s+.*(?:Formulas|Mathematics|Calculations|Principles)"),
    ("7. References", r"^##\s+7\.\s+.*(?:References|Documentation)"),
    ("8. Code Examples", r"^##\s+8\.\s+.*Code\s+Examples"),
    ("9. Practical Experiments", r"^##\s+9\.\s+.*Practical.*(?:Experiments|Laboratory)"),
    ("10. Common Mistakes", r"^##\s+10\.\s+.*Common\s+(?:Engineering\s+)?(?:Mistakes|Errors)"),
    ("11. Exercises", r"^##\s+11\.\s+.*Exercises"),
    ("12. Assessment", r"^##\s+12\.\s+.*Assessment"),
    ("13. Completion Criteria", r"^##\s+13\.\s+.*Completion\s+Criteria"),
]

AUTHORITATIVE_REF_RE = re.compile(r"\b(?:RM0440|DS12288|PM0214|UM2505|ES0430|ARM|ISO)\b")
STAGE_NAV_RE = re.compile(
    r"(?:Previous\s+Stage|Next\s+Stage|←\s*\[.*?\]\(\.\./|\[←[^\]]*\]\(\.\./|\[.*?\]\(\.\./[^)]+/README\.md\)\s*→|\[[^\]]*→\]\(\.\./|\*\*Prev(?:ious)?:\*\*|\*\*Next:\*\*)",
    re.I,
)


def check_curriculum_and_artifacts(repo_root: Path, rep: ValidationReport) -> None:
    section_errors: list[str] = []
    ref_errors: list[str] = []
    nav_warnings: list[str] = []

    for rel in STAGE_READMES:
        p = repo_root / rel
        if not p.is_file():
            section_errors.append(f"{rel}: file missing")
            continue
        txt = p.read_text(encoding="utf-8", errors="replace")
        missing_secs = [
            name for name, pat in CURRICULUM_REQUIRED_SECTIONS if not re.search(pat, txt, re.M | re.I)
        ]
        if missing_secs:
            section_errors.append(f"{rel}: missing {len(missing_secs)} section(s): {', '.join(missing_secs)}")
        if len(txt) < 5000:
            section_errors.append(f"{rel}: unexpectedly short ({len(txt)} < 5000 chars)")
        if not AUTHORITATIVE_REF_RE.search(txt):
            ref_errors.append(f"{rel}: no authoritative primary reference (RM0440/DS12288/PM0214/UM2505/ES0430/ARM/ISO)")
        if not STAGE_NAV_RE.search(txt):
            nav_warnings.append(f"{rel}: missing explicit Prev/Next stage navigation links")

    if section_errors:
        rep.err("curriculum", f"Curriculum 13-section audit FAILED ({len(section_errors)} stage(s))", section_errors)
    elif ref_errors:
        rep.err("curriculum", f"Curriculum authoritative reference audit FAILED ({len(ref_errors)} stage(s))", ref_errors)
    else:
        rep.info(
            "curriculum",
            f"All {len(STAGE_READMES)}/17 curriculum stage modules verified (13/13 sections + authoritative primary references).",
        )
    if nav_warnings:
        rep.warn(
            "curriculum_nav",
            f"{len(nav_warnings)}/{len(STAGE_READMES)} curriculum stage(s) missing Prev/Next navigation bar links",
            nav_warnings,
        )
    else:
        rep.info("curriculum_nav", f"All {len(STAGE_READMES)}/17 curriculum stages include Prev/Next navigation links.")

    # Drivers, FreeRTOS, Capstones, Video DB
    drv_dir = repo_root / "23-code-library" / "Drivers"
    missing_drv = [
        f"{name}{ext}"
        for name in REQUIRED_DRIVERS
        for ext in (".h", ".c")
        if not (drv_dir / f"{name}{ext}").is_file()
    ]
    if missing_drv:
        rep.err("drivers", f"Missing bare-metal driver files ({len(missing_drv)})", missing_drv)
    else:
        rep.info("drivers", f"All {len(REQUIRED_DRIVERS)}/12 bare-metal drivers (.h/.c) verified in 23-code-library/Drivers/.")

    rtos_ver = repo_root / "23-code-library" / "freertos" / "FreeRTOS-Kernel" / "VERSION.txt"
    rtos_mk = repo_root / "23-code-library" / "freertos" / "Makefile"
    if not (rtos_ver.is_file() and rtos_mk.is_file()):
        rep.err("freertos", "Pinned FreeRTOS-Kernel project files (VERSION.txt / Makefile) missing.")
    else:
        rep.info("freertos", "Pinned FreeRTOS-Kernel V11.1.0 project structure verified.")

    missing_caps = [d for d in CAPSTONE_DIRS if not (repo_root / d / "Makefile").is_file()]
    if missing_caps:
        rep.err("capstones", "Missing Capstone Makefiles", missing_caps)
    else:
        rep.info("capstones", f"All {len(CAPSTONE_DIRS)}/3 Capstone firmware projects verified in 19-projects/.")

    csv_path = repo_root / "22-video-library" / "video-database.csv"
    expected_video_cols = [
        "index",
        "title",
        "instructor_channel",
        "url",
        "platform",
        "duration",
        "publication_date",
        "curriculum_category",
        "curriculum_subtopic",
        "prerequisites",
        "educational_value",
        "missing_concepts",
    ]
    https_url_re = re.compile(r"^https://[A-Za-z0-9.-]+\.[A-Za-z]{2,}(?:/[^\s]*)?$")
    if not csv_path.is_file():
        rep.err("video_db", "22-video-library/video-database.csv missing.")
    else:
        with csv_path.open("r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            actual_cols = list(reader.fieldnames or [])
            rows = list(reader)

        csv_errors: list[str] = []
        if actual_cols != expected_video_cols:
            missing_cols = [c for c in expected_video_cols if c not in actual_cols]
            extra_cols = [c for c in actual_cols if c not in expected_video_cols]
            csv_errors.append(
                f"CSV schema column mismatch (missing={missing_cols}, extra={extra_cols}, actual={actual_cols})"
            )
        if len(rows) < 70:
            csv_errors.append(f"Expected >= 70 video entries, found {len(rows)}")

        seen_indices: set[str] = set()
        seen_urls: set[str] = set()
        seen_yt_ids: set[str] = set()
        for row_num, r in enumerate(rows, 2):
            idx_val = (r.get("index") or "").strip()
            url_val = (r.get("url") or "").strip()
            title_val = (r.get("title") or "").strip()
            if not idx_val:
                csv_errors.append(f"row {row_num}: missing 'index'")
            elif idx_val in seen_indices:
                csv_errors.append(f"row {row_num}: duplicate index '{idx_val}'")
            else:
                seen_indices.add(idx_val)

            if not title_val:
                csv_errors.append(f"row {row_num} (index={idx_val}): empty title")

            if not https_url_re.match(url_val):
                csv_errors.append(f"row {row_num} (index={idx_val}): invalid HTTPS URL '{url_val}'")
            elif url_val in seen_urls:
                csv_errors.append(f"row {row_num} (index={idx_val}): duplicate URL '{url_val}'")
            else:
                seen_urls.add(url_val)

            yt_m = re.search(r"v=([A-Za-z0-9_-]{11})", url_val)
            if yt_m:
                yt_id = yt_m.group(1)
                if yt_id in seen_yt_ids:
                    csv_errors.append(f"row {row_num} (index={idx_val}): duplicate YouTube ID '{yt_id}'")
                else:
                    seen_yt_ids.add(yt_id)

        rep.metrics["video_db"] = {
            "rows": len(rows),
            "columns_match": actual_cols == expected_video_cols,
            "unique_indices": len(seen_indices),
            "unique_urls": len(seen_urls),
            "unique_youtube_ids": len(seen_yt_ids),
            "errors": len(csv_errors),
        }
        if csv_errors:
            rep.err("video_db", f"Video DB schema/URL audit FAILED ({len(csv_errors)} issue(s))", csv_errors)
        else:
            rep.info(
                "video_db",
                f"Video database CSV schema verified: {len(rows)} entries (>=70), "
                f"{len(expected_video_cols)}/12 exact columns, {len(seen_indices)} unique IDs, "
                f"{len(seen_urls)} valid unique HTTPS URLs, 0 duplicate YouTube IDs.",
            )


# =============================================================================
# 8 & 9. Firmware Builds, ELF Inspection & Host Unit Tests
#        (Exact 9-State Taxonomy & ZERO Short-Circuiting — Section 4.G)
# =============================================================================
BUILD_TEST_STATUS_TAXONOMY = frozenset({
    "TOOLCHAIN_UNAVAILABLE",       # UNVERIFIED
    "BUILD_SKIPPED",               # UNVERIFIED (never PASS!)
    "BUILD_FAILED",                # ERROR
    "BUILD_PASSED",                # INFO/PASS
    "HOST_TESTS_SKIPPED",          # UNVERIFIED
    "HOST_TESTS_FAILED",           # ERROR
    "HOST_TESTS_PASSED",           # INFO/PASS
    "ELF_INSPECTION_UNAVAILABLE",  # UNVERIFIED
    "ELF_INSPECTION_PASSED",       # INFO/PASS
})


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


def check_register_map_c_snippets(
    repo_root: Path,
    rep: ValidationReport,
    gcc_arm: str | None,
    stage_states: dict[str, str] | None = None,
) -> str:
    """Compile standalone CMSIS C code blocks in 20-technical-diagrams/registers/*.md (v4.1.0 M7 + v4.2.0 9-state)."""
    def _record(state: str) -> str:
        if stage_states is not None:
            stage_states["register_snippets"] = state
        return state

    if not gcc_arm:
        rep.unverified_item(
            "register_snippets",
            "TOOLCHAIN_UNAVAILABLE: Register-map C snippet compilation unverified (missing required tool arm-none-eabi-gcc).",
        )
        return _record("TOOLCHAIN_UNAVAILABLE")

    reg_dir = repo_root / "20-technical-diagrams" / "registers"
    inc_dev = repo_root / "23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include"
    inc_core = repo_root / "23-code-library/Drivers/CMSIS/Core/Include"
    if not reg_dir.is_dir() or not inc_dev.is_dir() or not inc_core.is_dir():
        rep.err("register_snippets", "BUILD_FAILED: Register-map directory or CMSIS Include directories missing.")
        return _record("BUILD_FAILED")

    compiled = 0
    skipped = 0
    failures: list[str] = []
    with tempfile.TemporaryDirectory() as td:
        for md in sorted(reg_dir.glob("*.md")):
            txt = md.read_text(encoding="utf-8", errors="replace")
            for idx, m in enumerate(re.finditer(r"```c\r?\n(.*?)```", txt, re.S), 1):
                code = m.group(1)
                if '#include "stm32g474xx.h"' not in code and '#include "core_cm4.h"' not in code:
                    skipped += 1
                    continue
                compiled += 1
                src = Path(td) / f"{md.stem}_{idx}.c"
                src.write_text(code, encoding="utf-8")
                try:
                    proc = subprocess.run(
                        [
                            gcc_arm,
                            "-mcpu=cortex-m4",
                            "-mthumb",
                            "-mfpu=fpv4-sp-d16",
                            "-mfloat-abi=hard",
                            "-std=c11",
                            "-O2",
                            "-Wall",
                            "-Wextra",
                            "-Werror",
                            "-fsyntax-only",
                            "-DSTM32G474xx",
                            f"-I{inc_dev}",
                            f"-I{inc_core}",
                            str(src),
                        ],
                        capture_output=True,
                        text=True,
                    )
                    if proc.returncode != 0:
                        err_lines = (proc.stderr or proc.stdout).strip().splitlines()
                        err_line = next((ln for ln in err_lines if "error:" in ln), err_lines[0] if err_lines else "compile error")
                        err_line = re.sub(r"^.*?[\\/]([^\\/]+\.c:\d+:\d+:)", r"\1", err_line)
                        failures.append(f"{md.name} C-block #{idx}: {err_line}")
                except OSError as exc:
                    failures.append(f"{md.name} C-block #{idx}: {exc}")

    rep.metrics["register_snippets"] = {"compiled": compiled, "skipped_fragments": skipped, "failures": len(failures)}
    if failures:
        rep.err(
            "register_snippets",
            f"BUILD_FAILED: Register-map C snippet CMSIS compilation failed ({len(failures)}/{compiled} block(s))",
            failures,
        )
        return _record("BUILD_FAILED")
    if compiled == 0:
        rep.err("register_snippets", "BUILD_FAILED: No standalone CMSIS C code blocks found in 20-technical-diagrams/registers/*.md.")
        return _record("BUILD_FAILED")

    rep.info(
        "register_snippets",
        f"BUILD_PASSED: All {compiled}/{compiled} standalone CMSIS C snippets in 20-technical-diagrams/registers/*.md "
        f"syntax-compiled cleanly (-fsyntax-only -Wall -Wextra -Werror; {skipped} prose fragment(s) skipped).",
    )
    return _record("BUILD_PASSED")


def _run_independent_make_stage(
    stage_key: str,
    label: str,
    cwd: Path,
    make_args: list[str],
    make_exe: str | None,
    gcc_arm: str | None,
    env: dict[str, str] | None,
    rep: ValidationReport,
    stage_states: dict[str, str],
) -> bool:
    """Execute one independent firmware make stage without ever short-circuiting subsequent stages."""
    if not make_exe or not gcc_arm:
        missing = [t for t, p in (("make", make_exe), ("arm-none-eabi-gcc", gcc_arm)) if not p]
        stage_states[stage_key] = "TOOLCHAIN_UNAVAILABLE"
        rep.unverified_item(
            stage_key,
            f"TOOLCHAIN_UNAVAILABLE: {label} unverified (missing required tool(s): {', '.join(missing)}).",
        )
        return False

    if not cwd.is_dir():
        stage_states[stage_key] = "BUILD_FAILED"
        rep.err(stage_key, f"BUILD_FAILED: {label} directory missing ({cwd}).")
        return False

    try:
        proc = subprocess.run(
            [make_exe, *make_args],
            cwd=cwd,
            env=env,
            capture_output=True,
            text=True,
        )
    except OSError as exc:
        stage_states[stage_key] = "BUILD_FAILED"
        rep.err(stage_key, f"BUILD_FAILED: {label} invocation error ({exc}).")
        return False

    if proc.returncode != 0:
        stage_states[stage_key] = "BUILD_FAILED"
        rep.err(
            stage_key,
            f"BUILD_FAILED: {label} failed (exit {proc.returncode})",
            (proc.stderr or proc.stdout).splitlines()[-10:],
        )
        return False

    stage_states[stage_key] = "BUILD_PASSED"
    rep.info(stage_key, f"BUILD_PASSED: {label} compiled cleanly (exit 0).")
    return True


def check_builds_and_tests(repo_root: Path, rep: ValidationReport, skip_builds: bool = False) -> None:
    """
    Execute all 9 independent build, ELF inspection, and host unit-test stages with ZERO short-circuiting.
    Every stage always executes and records its exact standardized tag from the 9-state taxonomy.
    """
    stage_states: dict[str, str] = {}

    if skip_builds:
        skipped_build_stages = [
            ("register_snippets", "Register-map C snippet compilation"),
            ("firmware_build_template_debug", "Starter template + 12 drivers (BUILD_TYPE=debug)"),
            ("firmware_build_template_release", "Starter template + 12 drivers (BUILD_TYPE=release)"),
            ("firmware_build_freertos", "FreeRTOS V11.1.0 firmware (23-code-library/freertos/)"),
            ("firmware_build_capstone_01", f"Capstone 01 ({CAPSTONE_DIRS[0]})"),
            ("firmware_build_capstone_02", f"Capstone 02 ({CAPSTONE_DIRS[1]})"),
            ("firmware_build_capstone_03", f"Capstone 03 ({CAPSTONE_DIRS[2]})"),
        ]
        for key, label in skipped_build_stages:
            stage_states[key] = "BUILD_SKIPPED"
            rep.unverified_item(key, f"BUILD_SKIPPED: {label} skipped via --skip-builds.")
        stage_states["firmware_elf"] = "ELF_INSPECTION_UNAVAILABLE"
        rep.unverified_item(
            "firmware_elf",
            "ELF_INSPECTION_UNAVAILABLE: ELF .isr_vector inspection skipped via --skip-builds.",
        )
        stage_states["host_tests"] = "HOST_TESTS_SKIPPED"
        rep.unverified_item("host_tests", "HOST_TESTS_SKIPPED: Host C unit tests skipped via --skip-builds.")
        rep.metrics["build_stage_states"] = stage_states
        return

    make_exe = shutil.which("make") or find_arm_tool("make")
    gcc_arm = find_arm_tool("arm-none-eabi-gcc")
    readelf_arm = find_arm_tool("arm-none-eabi-readelf")

    env: dict[str, str] | None = None
    if make_exe and gcc_arm:
        env = os.environ.copy()
        arm_bin = str(Path(gcc_arm).parent)
        make_bin = str(Path(make_exe).parent)
        env["PATH"] = f"{arm_bin}{os.pathsep}{make_bin}{os.pathsep}{env.get('PATH', '')}"

    # Stage 1: Register-map C snippet CMSIS compile check (independent)
    check_register_map_c_snippets(repo_root, rep, gcc_arm, stage_states=stage_states)

    # Stage 2: Starter template debug build (independent — NEVER short-circuits release!)
    tpl_dir = repo_root / "23-code-library" / "templates"
    debug_ok = _run_independent_make_stage(
        "firmware_build_template_debug",
        "Starter template + 12 drivers (BUILD_TYPE=debug)",
        tpl_dir,
        ["BUILD_TYPE=debug", "clean", "all"],
        make_exe,
        gcc_arm,
        env,
        rep,
        stage_states,
    )

    # Stage 3: Starter template release build (independent — ALWAYS executes even if debug failed!)
    release_ok = _run_independent_make_stage(
        "firmware_build_template_release",
        "Starter template + 12 drivers (BUILD_TYPE=release)",
        tpl_dir,
        ["BUILD_TYPE=release", "clean", "all"],
        make_exe,
        gcc_arm,
        env,
        rep,
        stage_states,
    )

    # Stage 4: ELF .isr_vector inspection (independent — ALWAYS runs!)
    elf_path = tpl_dir / "build" / "firmware.elf"
    if not elf_path.is_file() and (tpl_dir / "build").is_dir():
        elves = list((tpl_dir / "build").rglob("*.elf"))
        if elves:
            elf_path = elves[0]

    if not readelf_arm:
        stage_states["firmware_elf"] = "ELF_INSPECTION_UNAVAILABLE"
        rep.unverified_item(
            "firmware_elf",
            "ELF_INSPECTION_UNAVAILABLE: arm-none-eabi-readelf not found on host.",
        )
    elif not make_exe or not gcc_arm:
        stage_states["firmware_elf"] = "ELF_INSPECTION_UNAVAILABLE"
        rep.unverified_item(
            "firmware_elf",
            "ELF_INSPECTION_UNAVAILABLE: firmware toolchain unavailable; ELF artifact not built.",
        )
    elif not elf_path.is_file():
        if not release_ok and not debug_ok:
            stage_states["firmware_elf"] = "ELF_INSPECTION_UNAVAILABLE"
            rep.unverified_item(
                "firmware_elf",
                "ELF_INSPECTION_UNAVAILABLE: firmware.elf not produced because starter template builds failed.",
            )
        else:
            stage_states["firmware_elf"] = "BUILD_FAILED"
            rep.err("firmware_elf", f"BUILD_FAILED: Expected ELF output missing at {elf_path.relative_to(repo_root)}.")
    else:
        try:
            re_proc = subprocess.run([readelf_arm, "-SW", str(elf_path)], capture_output=True, text=True)
            m = re.search(
                r"\.isr_vector\s+PROGBITS\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S+\s+\S+\s+\S+\s+\S+\s+(\d+)",
                re_proc.stdout,
            )
            if m:
                addr, size, align = int(m.group(1), 16), int(m.group(2), 16), int(m.group(3))
                if addr == 0x08000000 and size == 472 and align >= 512:
                    stage_states["firmware_elf"] = "ELF_INSPECTION_PASSED"
                    rep.info(
                        "firmware_elf",
                        f"ELF_INSPECTION_PASSED: ELF .isr_vector verified: addr=0x{addr:08X}, "
                        f"size={size} B (118 vectors), align={align} B.",
                    )
                else:
                    stage_states["firmware_elf"] = "BUILD_FAILED"
                    rep.err("firmware_elf", f"BUILD_FAILED: ELF .isr_vector invalid: addr=0x{addr:08X}, size={size}, align={align}")
            else:
                stage_states["firmware_elf"] = "BUILD_FAILED"
                rep.err("firmware_elf", "BUILD_FAILED: Could not parse .isr_vector section from readelf -SW output.")
        except OSError as exc:
            stage_states["firmware_elf"] = "ELF_INSPECTION_UNAVAILABLE"
            rep.unverified_item("firmware_elf", f"ELF_INSPECTION_UNAVAILABLE: readelf invocation error ({exc}).")

    # Stage 5: FreeRTOS V11.1.0 build (independent — ALWAYS executes!)
    rtos_dir = repo_root / "23-code-library" / "freertos"
    _run_independent_make_stage(
        "firmware_build_freertos",
        "FreeRTOS V11.1.0 firmware (23-code-library/freertos/)",
        rtos_dir,
        ["all"],
        make_exe,
        gcc_arm,
        env,
        rep,
        stage_states,
    )

    # Stages 6, 7, 8: All 3 Capstone builds (each executes independently!)
    for idx, cdir in enumerate(CAPSTONE_DIRS, 1):
        _run_independent_make_stage(
            f"firmware_build_capstone_{idx:02d}",
            f"Capstone {idx:02d} ({cdir})",
            repo_root / cdir,
            ["all"],
            make_exe,
            gcc_arm,
            env,
            rep,
            stage_states,
        )

    # Stage 9: Host C unit tests (independent — ALWAYS executes even if every firmware build failed!)
    host_gcc = shutil.which("gcc")
    if not host_gcc or not make_exe:
        missing = [t for t, p in (("gcc", host_gcc), ("make", make_exe)) if not p]
        stage_states["host_tests"] = "TOOLCHAIN_UNAVAILABLE"
        rep.unverified_item(
            "host_tests",
            f"TOOLCHAIN_UNAVAILABLE: Host C unit tests unverified (missing required tool(s): {', '.join(missing)}).",
        )
    else:
        tests_dir = repo_root / "tests"
        try:
            proc = subprocess.run(
                [make_exe, "-C", str(tests_dir), "clean", "test"],
                capture_output=True,
                text=True,
            )
            if proc.returncode != 0:
                stage_states["host_tests"] = "HOST_TESTS_FAILED"
                rep.err(
                    "host_tests",
                    f"HOST_TESTS_FAILED: Host C unit tests exited with code {proc.returncode}",
                    (proc.stderr or proc.stdout).splitlines()[-12:],
                )
            else:
                suite_matches = re.findall(
                    r"(\d+)\s+assertions?\s+executed,\s*(\d+)\s+failed",
                    proc.stdout,
                    re.IGNORECASE,
                )
                total_asserts = sum(int(a) for a, _ in suite_matches)
                total_failed = sum(int(f) for _, f in suite_matches)
                rep.metrics["host_tests"] = {
                    "suites": len(suite_matches),
                    "assertions_executed": total_asserts,
                    "assertions_failed": total_failed,
                }
                if total_failed != 0:
                    stage_states["host_tests"] = "HOST_TESTS_FAILED"
                    rep.err(
                        "host_tests",
                        f"HOST_TESTS_FAILED: Host C unit tests reported {total_failed} failed assertion(s) out of {total_asserts}.",
                    )
                elif total_asserts < 500 or len(suite_matches) < 4:
                    stage_states["host_tests"] = "HOST_TESTS_FAILED"
                    rep.err(
                        "host_tests",
                        f"HOST_TESTS_FAILED: Host C unit tests parsed {total_asserts} assertions across {len(suite_matches)} suites "
                        "(expected >= 500 across >= 4 suites).",
                    )
                else:
                    stage_states["host_tests"] = "HOST_TESTS_PASSED"
                    rep.info(
                        "host_tests",
                        f"HOST_TESTS_PASSED: {len(suite_matches)} suites, {total_asserts} assertions executed, 0 failed.",
                    )
        except OSError as exc:
            stage_states["host_tests"] = "HOST_TESTS_FAILED"
            rep.err("host_tests", f"HOST_TESTS_FAILED: invocation error ({exc}).")
        finally:
            try:
                subprocess.run([make_exe, "-C", str(tests_dir), "clean"], capture_output=True, text=True)
            except OSError:
                pass

    rep.metrics["build_stage_states"] = stage_states


def run_validation(
    repo_root: Path,
    skip_builds: bool = False,
    check_external_urls: bool = False,
    quiet: bool = False,
) -> ValidationReport:
    rep = ValidationReport(quiet=quiet)
    if not quiet:
        print(f"=== Embedded C & STM32G474RE Master Learning Hub Validator v{VALIDATOR_VERSION} ===")
        print(f"Repository: {repo_root}")
    check_structure(repo_root, rep)
    check_control_chars(repo_root, rep)
    check_cmsis_register_cross_check(repo_root, rep)
    check_sot_concepts(repo_root, rep)
    check_labs(repo_root, rep)
    ext_urls = check_markdown_links_and_mathjax(repo_root, rep)
    check_external_urls_live(ext_urls, rep, check_external_urls=check_external_urls)
    check_mermaid_diagrams(repo_root, rep)
    check_curriculum_and_artifacts(repo_root, rep)
    check_builds_and_tests(repo_root, rep, skip_builds=skip_builds)
    return rep


def main() -> int:
    parser = argparse.ArgumentParser(
        description=f"Regression-Resistant Validation Suite for the Embedded C & STM32G474RE Master Learning Hub (v{VALIDATOR_VERSION})."
    )
    parser.add_argument("--repo-root", type=Path, default=DEFAULT_REPO_ROOT, help="Path to repository root.")
    parser.add_argument("--skip-builds", action="store_true", help="Skip firmware build and host unit-test checks.")
    parser.add_argument(
        "--check-external-urls",
        action="store_true",
        help="Probe unique external HTTP(S) URLs live with polite concurrency and timeout.",
    )
    parser.add_argument("--json-report", type=Path, default=None, help="Write structured JSON validation report to path.")
    parser.add_argument("--quiet", action="store_true", help="Suppress per-check stdout output (summary still printed).")
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    rep = run_validation(
        repo_root=repo_root,
        skip_builds=args.skip_builds,
        check_external_urls=args.check_external_urls,
        quiet=args.quiet,
    )

    print(
        f"=== Validation Summary: "
        f"{len(rep.infos)} INFO/PASS | "
        f"{len(rep.warnings)} WARNING | "
        f"{len(rep.unverified)} UNVERIFIED | "
        f"{len(rep.errors)} ERROR ==="
    )

    if args.json_report is not None:
        args.json_report.parent.mkdir(parents=True, exist_ok=True)
        args.json_report.write_text(json.dumps(rep.to_dict(repo_root), indent=2), encoding="utf-8")
        print(f"JSON report written to: {args.json_report}")

    return 1 if rep.errors else 0


if __name__ == "__main__":
    sys.exit(main())
