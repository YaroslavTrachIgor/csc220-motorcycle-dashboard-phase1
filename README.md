# BAZOOKI OS — Motorcycle Dashboard (Phase I)

[![C](https://img.shields.io/badge/language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Build](https://img.shields.io/badge/build-Make-lightgrey.svg)](Makefile)

## Purpose

This repository implements **Phase I** of a simulated motorcycle “operating system”: a multi-threaded C program that models engine, motion, fuel, ECU, and dashboard subsystems, shares a single system state, and renders a live ASCII dashboard in the terminal. The goal is **threaded design**, **clear subsystem boundaries**, and **extensible architecture** for later phases (synchronization, I/O, shutdown).

## Course & citation

This work is submitted for:

- **Course:** CSC 220 — *Operating Systems and Systems Programming*  
- **Term:** Spring ’26  
- **Phase:** Phase I — *Threads & architecture*  
- **Assignment reference:** *Motorcycle Dashboard* — course document **OS Project _ Motorcycle _ Phase I.pdf** (distributed via course LMS / Canvas).

If you cite this project academically, please include the course code, term, phase title, and the PDF filename above.

## Features

- Five `pthread` subsystems: Engine, Motion, Fuel, ECU, Dashboard  
- Shared global state (`g_state`) with documented responsibilities per layer  
- Terminal UI with fixed-width borders, gauges, and status indicators  
- No GUI; ANSI clear/refresh for in-place updates  

## Prerequisites

- **Compiler:** GCC or Clang with C99 support  
- **Libraries:** POSIX threads (`pthread`) and standard math (`libm`)  
- **OS:** macOS or Linux with a UTF-8–capable terminal (for box-drawing characters)

## Installation

Clone the repository and build from the project root:

```bash
git clone https://github.com/<YOUR_USERNAME>/csc220-motorcycle-dashboard-phase1.git
cd csc220-motorcycle-dashboard-phase1
make
```

The binary `bazooki_os` is produced in the project root.

### Build artifacts

Object files are written to `build/`. To clean:

```bash
make clean
```

## Usage

```bash
./bazooki_os
```

Press **Ctrl+C** to stop the process. Phase I does not implement graceful thread shutdown.

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

Collaborator invitations are issued on GitHub so these accounts can push to the repository.

## License

Course assignment code unless otherwise specified by your instructor. Use and redistribution should follow your institution’s academic integrity policy.
