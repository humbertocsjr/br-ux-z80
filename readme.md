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

# BrFS - Brazilian File System

Simplified File System with partial POSIX Support.

Block ID is the block position in disk.

The first 6 blocks of the disk are not indexed, and are reserved for the Boot Loader, with Root positioned in block ID 6.

# Modes

Directory Mode    = 0x41ff
Regular File Mode = 0x81ff
Data Block Mode   = 0x0000

# Date Time Format

| Size | Description |
|------|-------------|
|    2 | Year        |
|    1 | Month       |
|    1 | Day         |
|    1 | Hour        |
|    1 | Minute      |
|    1 | Second      |
|    1 | Centisecond |

# Item Block Format

| Size | Description            |
|------|------------------------|
|   64 | Item Name              |
|    2 | Always Zero            |
|    2 | User ID                |
|    2 | Group ID               |
|    2 | Creation User ID       |
|    8 | Creation Date Time     |
|    8 | Modification Date Time |
|  412 | Reserved               |
|    2 | Block ID               |
|    2 | Mode                   |
|    2 | Next ID                |
|    2 | First Child ID         |
|    2 | Parent ID              |
|    2 | Data Size              |

# Root Block Format

| Size | Description            |
|------|------------------------|
|   64 | Item Name              |
|    2 | Always Zero            |
|    2 | User ID                |
|    2 | Group ID               |
|    2 | Creation User ID       |
|    8 | Creation Date Time     |
|    8 | Modification Date Time |
|  412 | Reserved               |
|    2 | Block ID               |
|    2 | Mode                   |
|    2 | Last Disk Block ID     |
|    2 | First Child ID         |
|    2 | Zero                   |
|    2 | Total Disk Blocks      |

# Data Block Format

| Size | Description       |
|------|-------------------|
|  500 | Raw Data          |
|    2 | Block ID          |
|    2 | Mode              |
|    2 | Next ID           |
|    2 | First Child ID    |
|    2 | Parent ID         |
|    2 | Data Size         |

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