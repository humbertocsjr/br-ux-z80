# BR-UX - Brazillian Unique *nix-Like Operating System

**Disclaimer:** The colors and name of the project are solely linked to the country and not to political spectrum or ideology.

# Versioning

Inpired on HP-UX Versioning System:

[Version][Edition] v[Version].[Sub version] B[Sub version][Revision]

- Version: Number
- Edition: 
    - a: Alpha
    - b: Beta
    - z: Production
- Sub version: 1 Digit Number
- Revision: 2 Hex Digits Number

**Examples.:**

- 1a v1.1 R100
- 4b v4.5 R532
- 2z v2.1 R1f4

# BrazuxFS - Brazux File System

Simplified File System with partial POSIX Support.

# Target | MSX 1

This version aims to support the base model of the MSX 1.

## Minimum Requirements

- MSX 1 Machine or Compatible
- 16 KiB RAM
- 720 KiB Floppy Drive
- MegaRAM 256 KiB

## MegaRAM Layout

| Page | Description |
|------|-------------|
|    0 | Kernel Data |
|    1 | Kernel      |

## RAM Layout

| Segment | Description |
|---------|-------------|
| 0x0000  | BIOS        |
| 0x4000  | Application |
| 0x6000  | Data        |
| 0x8000  | Stack       |
| 0xa000  | Kernel Data |
| 0xc000  | Kernel      |