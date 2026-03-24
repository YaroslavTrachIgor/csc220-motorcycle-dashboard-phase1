# BAZOOKI OS — Motorcycle Dashboard (Phase I)

[![C](https://img.shields.io/badge/language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Build](https://img.shields.io/badge/build-Make-lightgrey.svg)](Makefile)

## Purpose

This repository implements **Phase I** of a simulated motorcycle “operating system”: a multi-threaded C program that models engine, motion, fuel, ECU, and dashboard subsystems, shares a single system state, and renders a live ASCII dashboard in the terminal. The goal is **threaded design**, **clear subsystem boundaries**, and **extensible architecture** for later phases (synchronization, I/O, shutdown).

## Course & citation

University of Tampa, CSC 220, Spring 2026, Phase I assignment *Motorcycle Dashboard*; specification PDF *OS Project _ Motorcycle _ Phase I.pdf*.

## Prerequisites

- **Compiler:** GCC or Clang with C99 support  
- **Libraries:** POSIX threads (`pthread`) and standard math (`libm`)  
- **OS:** macOS or Linux with a UTF-8–capable terminal (for box-drawing characters)

## Installation

Obtain the source (clone or archive) and `cd` to the project root.

### 1) Makefile — `build/` and `bazooki_os`

The default build uses **Make**. Object files go under **`build/`**; the executable **`bazooki_os`** is written to the **project root**.

```bash
git clone https://github.com/YaroslavTrachIgor/csc220-motorcycle-dashboard-phase1.git
cd csc220-motorcycle-dashboard-phase1
make
```

| Output | Location |
|--------|----------|
| Object files (`.o`) | `build/` |
| Executable | `./bazooki_os` |

```bash
make clean   # removes build/ and bazooki_os
```

### 2) GCC commands (no Make)

Equivalent to the Makefile: compile each translation unit, then link with `-pthread` and `-lm`.

```bash
mkdir -p build
gcc -Wall -Wextra -std=c99 -I include -pthread -c src/main.c         -o build/main.o
gcc -Wall -Wextra -std=c99 -I include -pthread -c src/system_state.c   -o build/system_state.o
gcc -Wall -Wextra -std=c99 -I include -pthread -c src/engine.c         -o build/engine.o
gcc -Wall -Wextra -std=c99 -I include -pthread -c src/motion.c         -o build/motion.o
gcc -Wall -Wextra -std=c99 -I include -pthread -c src/fuel.c           -o build/fuel.o
gcc -Wall -Wextra -std=c99 -I include -pthread -c src/ecu.c            -o build/ecu.o
gcc -Wall -Wextra -std=c99 -I include -pthread -c src/dashboard.c      -o build/dashboard.o
gcc build/main.o build/system_state.o build/engine.o build/motion.o \
    build/fuel.o build/ecu.o build/dashboard.o -o bazooki_os -pthread -lm
```

Single-shot alternative (same flags, all sources at once):

```bash
gcc -Wall -Wextra -std=c99 -I include -pthread src/*.c -o bazooki_os -lm
```

### Notes: GCC on Ubuntu inside Docker

These steps match a typical classroom setup: **Ubuntu** image, **GCC** from the distribution, project built at a shell inside the container.

1. Run an interactive Ubuntu container (example):

   ```bash
   docker run -it --rm ubuntu:22.04 bash
   ```

2. Install the toolchain (provides `gcc`, `make`, and common build headers):

   ```bash
   apt-get update
   apt-get install -y build-essential git
   ```

3. Copy or clone the project into the container, `cd` to the project root, then build using **Makefile — `build/` and `bazooki_os`** or **GCC commands (no Make)** above.

4. **Linking:** The final link step must include **`-pthread`** (POSIX threads) and **`-lm`** (math). On Ubuntu, `-pthread` is required for both compile and link when using `pthread` APIs.

5. **Terminal:** Use an interactive TTY (`-it`) so the dashboard redraws correctly. Box-drawing characters assume a UTF-8 locale; if borders render incorrectly, set e.g. `export LANG=C.UTF-8` before running `./bazooki_os`.

## Usage

```bash
./bazooki_os
```

**Ctrl+C** terminates the process. Phase I does not implement graceful thread shutdown.

## Architecture

The design follows three layers:

| Layer | Subsystems | Responsibility |
|-------|------------|----------------|
| **Simulation** | Engine, Motion, Fuel | Update physical / resource state over time |
| **Control** | ECU | Classify and derive state from sensor-like inputs |
| **Presentation** | Dashboard | Read-only visualization; no simulation logic |

### Subsystems (summary)

| Subsystem | Source | Role |
|-----------|--------|------|
| Engine | `engine.c` | RPM, temperature vs. load |
| Motion | `motion.c` | Speed profile, distance accumulation |
| Fuel | `fuel.c` | Consumption tied to speed/RPM |
| ECU | `ecu.c` | RPM zone, temperature class, derived flags |
| Dashboard | `dashboard.c` | Formatted terminal output |

Phase I intentionally omits synchronization primitives; shared state access is documented for Phase II.

## Project layout

```
├── include/
│   ├── system_state.h
│   └── subsystems.h
├── src/
│   ├── main.c
│   ├── system_state.c
│   ├── engine.c
│   ├── motion.c
│   ├── fuel.c
│   ├── ecu.c
│   └── dashboard.c
├── Makefile
└── README.md
```

## Roadmap

- **Phase II:** Synchronization, system rules, safety-oriented ECU behavior  
- **Phase III:** I/O, integration, orderly shutdown  

## Contributors

| Contributor | GitHub |
|-------------|--------|
| asheehyut | [@asheehyut](https://github.com/asheehyut) |
| Y1LD1Z-US | [@Y1LD1Z-US](https://github.com/Y1LD1Z-US) |

## License & academic notice

This work is submitted in connection with **The University of Tampa** and **CSC 220 — Operating Systems and Systems Programming**. It is **not** released under an open-source software license. Ownership, permitted use, and any redistribution are subject to **The University of Tampa** policies, including the **Honor Code** and **academic integrity** requirements, and to the terms set by the course instructor. Commercial use, publication as one’s own without attribution where required, or reuse beyond course scope is not authorized except as explicitly allowed by university and course policy.
