#!/usr/bin/env python3
"""
Deterministic SPDX 2.3 JSON SBOM Generator & Integrity Checker
for embedded-c-master-learning-hub.

Inventories all vendored third-party firmware components directly from disk:
  1. ARM CMSIS-Core (Cortex-M)                  — Apache-2.0
  2. STMicroelectronics CMSIS Device STM32G4xx  — BSD-3-Clause
  3. FreeRTOS-Kernel (ARM_CM4F + heap_4)        — MIT

Determinism Guarantee:
  - Uses canonical LF line endings (CRLF -> LF) prior to computing SHA-1 and
    SHA-256 digests so outputs are bit-for-bit identical across Windows and
    Linux CI checkouts regardless of git core.autocrlf settings.
  - Uses a fixed canonical creationInfo.created timestamp tied to the schema
    release (overrideable via --created) so `generate_sbom.py --check` is
    100% reproducible in CI.

Usage:
  python3 scripts/generate_sbom.py            # Generate SBOM/spdx-sbom.json
  python3 scripts/generate_sbom.py --check    # Verify zero drift in CI
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT = REPO_ROOT / "SBOM" / "spdx-sbom.json"
CANONICAL_CREATED = "2026-11-18T00:00:00Z"
REPO_VERSION = "4.0.0"
DOCUMENT_NAMESPACE = (
    "https://github.com/embedded-c-master-learning-hub/spdx/"
    f"embedded-c-master-learning-hub-{REPO_VERSION}"
)


def canonical_bytes(path: Path) -> bytes:
    """Read file bytes with CRLF -> LF normalization for OS-independent hashing."""
    raw = path.read_bytes()
    return raw.replace(b"\r\n", b"\n")


def sha1_hex(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def spdx_verification_code(file_sha1s: list[str]) -> str:
    """Compute SPDX 2.3 packageVerificationCodeValue per Annex J."""
    concatenated = "".join(sorted(h.lower() for h in file_sha1s)).encode("ascii")
    return hashlib.sha1(concatenated).hexdigest()


def sanitize_spdx_id(prefix: str, rel_path: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9.-]", "-", rel_path)
    return f"SPDXRef-{prefix}-{cleaned}"


def extract_cmsis_core_version(repo_root: Path) -> str:
    hdr = repo_root / "23-code-library/Drivers/CMSIS/Core/Include/cmsis_version.h"
    text = hdr.read_text(encoding="utf-8")
    m_main = re.search(r"#define\s+__CM_CMSIS_VERSION_MAIN\s*\(\s*(\d+)U?\s*\)", text)
    m_sub = re.search(r"#define\s+__CM_CMSIS_VERSION_SUB\s*\(\s*(\d+)U?\s*\)", text)
    if not m_main or not m_sub:
        raise RuntimeError(f"Could not parse CMSIS-Core version from {hdr}")
    return f"{m_main.group(1)}.{m_sub.group(1)}.0"


def extract_stm32g4_cmsis_version(repo_root: Path) -> str:
    hdr = repo_root / "23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g4xx.h"
    text = hdr.read_text(encoding="utf-8")
    m_main = re.search(r"#define\s+__STM32G4_CMSIS_VERSION_MAIN\s*\(\s*0x([0-9A-Fa-f]+)U?\s*\)", text)
    m_sub1 = re.search(r"#define\s+__STM32G4_CMSIS_VERSION_SUB1\s*\(\s*0x([0-9A-Fa-f]+)U?\s*\)", text)
    m_sub2 = re.search(r"#define\s+__STM32G4_CMSIS_VERSION_SUB2\s*\(\s*0x([0-9A-Fa-f]+)U?\s*\)", text)
    if not (m_main and m_sub1 and m_sub2):
        raise RuntimeError(f"Could not parse STM32G4 CMSIS Device version from {hdr}")
    return f"{int(m_main.group(1), 16)}.{int(m_sub1.group(1), 16)}.{int(m_sub2.group(1), 16)}"


def extract_freertos_version_and_sha(repo_root: Path) -> tuple[str, str]:
    task_h = repo_root / "23-code-library/freertos/FreeRTOS-Kernel/include/task.h"
    ver_txt = repo_root / "23-code-library/freertos/FreeRTOS-Kernel/VERSION.txt"
    m_ver = re.search(
        r'#define\s+tskKERNEL_VERSION_NUMBER\s+"([^"]+)"',
        task_h.read_text(encoding="utf-8"),
    )
    m_sha = re.search(
        r"Commit SHA\s*:\s*([0-9a-f]{40})",
        ver_txt.read_text(encoding="utf-8"),
    )
    if not m_ver or not m_sha:
        raise RuntimeError("Could not parse FreeRTOS-Kernel version or commit SHA")
    return m_ver.group(1), m_sha.group(1)


def collect_files(repo_root: Path, paths: list[str]) -> list[Path]:
    collected: list[Path] = []
    for rel in paths:
        target = repo_root / rel
        if not target.exists():
            raise FileNotFoundError(f"Expected vendored path missing: {rel}")
        if target.is_file():
            collected.append(target)
        else:
            for p in sorted(target.rglob("*")):
                if p.is_file():
                    collected.append(p)
    # Deduplicate and sort lexicographically by POSIX relative path
    unique = {p.resolve(): p for p in collected}
    return sorted(unique.values(), key=lambda p: p.relative_to(repo_root).as_posix())


def build_sbom(repo_root: Path, created: str) -> dict[str, Any]:
    cmsis_core_ver = extract_cmsis_core_version(repo_root)
    stm32g4_ver = extract_stm32g4_cmsis_version(repo_root)
    freertos_ver, freertos_sha = extract_freertos_version_and_sha(repo_root)

    component_specs = [
        {
            "spdx_id": "SPDXRef-Package-CMSIS-Core",
            "name": "ARM-CMSIS-Core",
            "version": cmsis_core_ver,
            "supplier": "Organization: ARM Limited",
            "originator": "Organization: ARM Limited",
            "download_location": "https://github.com/ARM-software/CMSIS_5",
            "homepage": "https://arm-software.github.io/CMSIS_5/Core/html/index.html",
            "license": "Apache-2.0",
            "copyright": "Copyright (c) 2009-2022 ARM Limited. All rights reserved.",
            "purl": f"pkg:github/ARM-software/CMSIS_5@{cmsis_core_ver}",
            "summary": "ARM CMSIS-Core (Cortex-M4) minimal vendored header subset (core_cm4.h, cmsis_version.h, cmsis_compiler.h, cmsis_gcc.h, mpu_armv7.h).",
            "comment": "LF-canonicalized digests for cross-OS reproducibility. Header banners: cmsis_version.h V5.0.5 (__CM_CMSIS_VERSION 5.6), core_cm4.h V5.1.2, cmsis_compiler.h V5.1.0, cmsis_gcc.h V5.4.1, mpu_armv7.h V5.1.2.",
            "paths": [
                "23-code-library/Drivers/CMSIS/Core/Include",
            ],
        },
        {
            "spdx_id": "SPDXRef-Package-STM32G4-CMSIS-Device",
            "name": "STM32G4xx-CMSIS-Device",
            "version": stm32g4_ver,
            "supplier": "Organization: STMicroelectronics",
            "originator": "Organization: STMicroelectronics",
            "download_location": f"https://github.com/STMicroelectronics/cmsis_device_g4/releases/tag/v{stm32g4_ver}",
            "homepage": "https://github.com/STMicroelectronics/cmsis_device_g4",
            "license": "BSD-3-Clause",
            "copyright": "Copyright (c) 2019 STMicroelectronics. All rights reserved.",
            "purl": f"pkg:github/STMicroelectronics/cmsis_device_g4@v{stm32g4_ver}",
            "summary": "STMicroelectronics CMSIS Device Peripheral Access Layer for STM32G474xx (stm32g4xx.h, stm32g474xx.h, system_stm32g4xx.h, system_stm32g4xx.c).",
            "comment": f"LF-canonicalized digests for cross-OS reproducibility. Verified __STM32G4_CMSIS_VERSION = V{stm32g4_ver} in stm32g4xx.h.",
            "paths": [
                "23-code-library/Drivers/CMSIS/Device/ST/STM32G4xx/Include",
                "23-code-library/templates/Src/system_stm32g4xx.c",
            ],
        },
        {
            "spdx_id": "SPDXRef-Package-FreeRTOS-Kernel",
            "name": "FreeRTOS-Kernel",
            "version": freertos_ver,
            "supplier": "Organization: Amazon Web Services, Inc.",
            "originator": "Organization: Real Time Engineers Ltd. / Amazon.com, Inc.",
            "download_location": f"git+https://github.com/FreeRTOS/FreeRTOS-Kernel.git@{freertos_sha}",
            "homepage": "https://www.freertos.org/",
            "license": "MIT",
            "copyright": "Copyright (C) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.",
            "purl": f"pkg:github/FreeRTOS/FreeRTOS-Kernel@{freertos_ver}",
            "summary": "FreeRTOS-Kernel V11.1.0 minimal Cortex-M4F GCC port (ARM_CM4F) and heap_4 allocator subset.",
            "comment": f"LF-canonicalized digests for cross-OS reproducibility. Upstream release {freertos_ver}, commit {freertos_sha}.",
            "paths": [
                "23-code-library/freertos/FreeRTOS-Kernel",
            ],
        },
    ]

    packages: list[dict[str, Any]] = [
        {
            "SPDXID": "SPDXRef-Package-Root",
            "name": "embedded-c-master-learning-hub",
            "versionInfo": REPO_VERSION,
            "supplier": "Organization: Embedded C Master Learning Hub Contributors",
            "downloadLocation": "NOASSERTION",
            "filesAnalyzed": False,
            "licenseConcluded": "MIT",
            "licenseDeclared": "MIT",
            "copyrightText": "Copyright (c) 2026 Embedded C Master Learning Hub Contributors",
            "summary": "Bare-Metal Embedded C & STM32G474RE (NUCLEO-G474RE) Master Learning Hub curriculum, bare-metal drivers, FreeRTOS reference firmware, and capstone projects.",
            "primaryPackagePurpose": "APPLICATION",
        }
    ]

    files_out: list[dict[str, Any]] = []
    relationships: list[dict[str, str]] = [
        {
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relationshipType": "DESCRIBES",
            "relatedSpdxElement": "SPDXRef-Package-Root",
        }
    ]

    for spec in component_specs:
        pkg_files = collect_files(repo_root, spec["paths"])
        file_sha1s: list[str] = []

        for fpath in pkg_files:
            rel_posix = fpath.relative_to(repo_root).as_posix()
            data = canonical_bytes(fpath)
            h1 = sha1_hex(data)
            h256 = sha256_hex(data)
            file_sha1s.append(h1)

            file_spdx_id = sanitize_spdx_id("File", rel_posix)
            files_out.append(
                {
                    "SPDXID": file_spdx_id,
                    "fileName": f"./{rel_posix}",
                    "checksums": [
                        {"algorithm": "SHA1", "checksumValue": h1},
                        {"algorithm": "SHA256", "checksumValue": h256},
                    ],
                    "licenseConcluded": spec["license"],
                    "licenseInfoInFiles": [spec["license"]],
                    "copyrightText": spec["copyright"],
                }
            )
            relationships.append(
                {
                    "spdxElementId": spec["spdx_id"],
                    "relationshipType": "CONTAINS",
                    "relatedSpdxElement": file_spdx_id,
                }
            )

        packages.append(
            {
                "SPDXID": spec["spdx_id"],
                "name": spec["name"],
                "versionInfo": spec["version"],
                "supplier": spec["supplier"],
                "originator": spec["originator"],
                "downloadLocation": spec["download_location"],
                "homepage": spec["homepage"],
                "filesAnalyzed": True,
                "packageVerificationCode": {
                    "packageVerificationCodeValue": spdx_verification_code(file_sha1s),
                },
                "licenseConcluded": spec["license"],
                "licenseDeclared": spec["license"],
                "licenseInfoFromFiles": [spec["license"]],
                "copyrightText": spec["copyright"],
                "summary": spec["summary"],
                "comment": spec["comment"],
                "primaryPackagePurpose": "LIBRARY",
                "externalRefs": [
                    {
                        "referenceCategory": "PACKAGE-MANAGER",
                        "referenceType": "purl",
                        "referenceLocator": spec["purl"],
                    }
                ],
            }
        )

        relationships.append(
            {
                "spdxElementId": "SPDXRef-Package-Root",
                "relationshipType": "CONTAINS",
                "relatedSpdxElement": spec["spdx_id"],
            }
        )

    return {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"embedded-c-master-learning-hub-{REPO_VERSION}-sbom",
        "documentNamespace": DOCUMENT_NAMESPACE,
        "creationInfo": {
            "created": created,
            "creators": [
                "Tool: embedded-c-master-learning-hub/scripts/generate_sbom.py-1.0.0",
            ],
            "licenseListVersion": "3.25",
            "comment": "Deterministic SPDX 2.3 SBOM covering all vendored third-party C firmware components. Text files are LF-canonicalized prior to SHA-1/SHA-256 hashing for Windows/Linux CI parity.",
        },
        "documentDescribes": ["SPDXRef-Package-Root"],
        "packages": packages,
        "files": files_out,
        "relationships": relationships,
    }


def validate_spdx_structure(doc: dict[str, Any]) -> None:
    """Perform strict internal structural validation on the SPDX 2.3 JSON document."""
    required_top = [
        "spdxVersion",
        "dataLicense",
        "SPDXID",
        "name",
        "documentNamespace",
        "creationInfo",
        "packages",
        "files",
        "relationships",
    ]
    for key in required_top:
        if key not in doc:
            raise ValueError(f"Missing required SPDX 2.3 field: {key}")
    if doc["spdxVersion"] != "SPDX-2.3":
        raise ValueError(f"Unexpected spdxVersion: {doc['spdxVersion']}")
    if doc["dataLicense"] != "CC0-1.0":
        raise ValueError(f"Unexpected dataLicense: {doc['dataLicense']}")

    known_ids = {"SPDXRef-DOCUMENT"}
    for pkg in doc["packages"]:
        sid = pkg["SPDXID"]
        if sid in known_ids:
            raise ValueError(f"Duplicate SPDXID: {sid}")
        known_ids.add(sid)
    for fobj in doc["files"]:
        sid = fobj["SPDXID"]
        if sid in known_ids:
            raise ValueError(f"Duplicate SPDXID: {sid}")
        known_ids.add(sid)
    for rel in doc["relationships"]:
        if rel["spdxElementId"] not in known_ids:
            raise ValueError(f"Unknown spdxElementId in relationship: {rel['spdxElementId']}")
        if rel["relatedSpdxElement"] not in known_ids:
            raise ValueError(f"Unknown relatedSpdxElement in relationship: {rel['relatedSpdxElement']}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate or verify deterministic SPDX 2.3 JSON SBOM for vendored firmware components."
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Output path for SPDX 2.3 JSON file (default: SBOM/spdx-sbom.json).",
    )
    parser.add_argument(
        "--created",
        default=CANONICAL_CREATED,
        help=f"ISO 8601 UTC timestamp for creationInfo.created (default: {CANONICAL_CREATED}).",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Verify that the existing SBOM file matches the deterministic generated output.",
    )
    args = parser.parse_args()

    doc = build_sbom(REPO_ROOT, args.created)
    validate_spdx_structure(doc)
    rendered = json.dumps(doc, indent=2, ensure_ascii=False) + "\n"

    out_path = args.output if args.output.is_absolute() else (REPO_ROOT / args.output)
    third_party_pkgs = [p for p in doc["packages"] if p["SPDXID"] != "SPDXRef-Package-Root"]

    if args.check:
        if not out_path.exists():
            print(f"[SBOM-CHECK FAIL] Missing SBOM file: {out_path}", file=sys.stderr)
            return 1
        existing = out_path.read_text(encoding="utf-8").replace("\r\n", "\n")
        if existing != rendered:
            print(
                f"[SBOM-CHECK FAIL] {out_path} is out of date with vendored components. "
                "Run 'python3 scripts/generate_sbom.py' to regenerate.",
                file=sys.stderr,
            )
            return 1
        print(
            f"[SBOM-CHECK PASS] {out_path.relative_to(REPO_ROOT).as_posix()} verified: "
            f"SPDX-2.3, {len(doc['packages'])} packages ({len(third_party_pkgs)} vendored third-party), "
            f"{len(doc['files'])} files, {len(doc['relationships'])} relationships."
        )
        return 0

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8", newline="\n") as fh:
        fh.write(rendered)

    print(
        f"[SBOM-GEN PASS] Wrote {out_path.relative_to(REPO_ROOT).as_posix()}: "
        f"SPDX-2.3, {len(doc['packages'])} packages ({len(third_party_pkgs)} vendored third-party), "
        f"{len(doc['files'])} files, {len(doc['relationships'])} relationships."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
