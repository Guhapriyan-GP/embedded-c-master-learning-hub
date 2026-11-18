# Local Source-Code Backup Manifest

**Backup Authority:** Agent 0 — Master Coordinator  
**Creation Timestamp:** `2026-11-18T16:41:10+05:30`  
**Status:** **VERIFIED & IMMUTABLE** *(Do not overwrite this verified baseline backup)*

---

## 1. Backup Metadata

| Field | Verified Value |
|---|---|
| **Working Repository** | `D:\General_ant\embedded-c-master-learning-hub` |
| **Independent Local Backup Path** | `D:\General_ant\embedded-c-master-learning-hub-backup` |
| **Source Git Branch** | `main` (clean working tree) |
| **Source Git Commit SHA** | `585dc4fa081cf7f27e5e437254e814f13f4b46cd` |
| **Git Metadata Preserved** | Yes (complete `.git/` history, refs, objects, hooks, and config copied) |
| **Total Files Copied & Verified** | `565` files |
| **Total Byte Size Verified** | `5,734,859` bytes |
| **SHA-256 File Mismatches (Source vs Backup)** | `0` (`565 / 565` byte-for-byte identical) |
| **Aggregate Manifest SHA-256** | `84aa5ef482853df5f0af1e4c401bd6a3104fca9fe9e182def4ca6530ab015cb5` |

---

## 2. Documented Exclusions (Reproducible Build Artifacts Only)

Only temporary/reproducible generated directories and build outputs were excluded from the source backup:
- **Directories excluded:** `build/`, `__pycache__/`, `.pytest_cache/`
- **File extensions excluded:** `*.o`, `*.obj`, `*.elf`, `*.bin`, `*.hex`, `*.map`, `*.d`, `*.su`, `*.exe`, `*.pyc`
- **All source files preserved:** `.c`, `.h`, `.ld`, `.md`, `.mmd`, `.json`, `.csv`, `.py`, `Makefile`, `CMakeLists.txt`, `.vscode/`, `.github/`, `.clang-format`, `.gdbinit`, `.gitignore`, `LICENSE`, and full `.git/` metadata.

---

## 3. Restoration Verification Test

To prove the backup is genuinely restorable and uncorrupted, an independent restoration test was executed into an isolated temporary directory (`%TEMP%\hub_restore_test_*`):

| Verification Check | Result |
|---|---|
| **Restored File Count** | `565 / 565` (`5,734,859` bytes) |
| **SHA-256 Mismatches (Backup vs Restored)** | `0` |
| **Restored `git rev-parse HEAD`** | `585dc4fa081cf7f27e5e437254e814f13f4b46cd` (**EXACT MATCH**) |
| **Restored `git status --short`** | Clean (0 modified / 0 untracked files) |
| **Temporary Test Dir Cleanup** | Removed cleanly after verification |

---

## 4. Hardware Flash Recovery Snapshot

Prior to any onboard hardware validation on the connected **NUCLEO-G474RE** (STLINK-V3E `V3J17M11`, Device ID `0x469`, Rev X, 512 KB Flash, 3.28 V), a complete read-only 512 KB Flash backup was captured via `STM32_Programmer_CLI v2.23.0`:
- **Command:** `STM32_Programmer_CLI -c port=SWD mode=NORMAL -u 0x08000000 0x80000 <recovery_image.bin>`
- **Size:** `524,288` bytes (`0x08000000`–`0x0807FFFF`) — **Data read successfully (1.351 s)**
- **Safety Guarantee:** Option bytes, RDP, WRP, boot configuration, and ST-LINK firmware remain completely unmodified.

---

## 5. Re-Verification Procedure (Phase 9 Final Check)

At the end of remediation, verify that the baseline backup has never been altered:
```powershell
python -c "import hashlib, pathlib; b = pathlib.Path(r'D:\General_ant\embedded-c-master-learning-hub-backup'); files = sorted([p for p in b.rglob('*') if p.is_file()]); print('Files:', len(files), 'Bytes:', sum(p.stat().st_size for p in files))"
```
Expected: `Files: 565 Bytes: 5734859`.
