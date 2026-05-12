# CENG302 Project 2

This repository is a ready-to-build starter for the Operating Systems project on classic synchronization problems.

## Contents

- `it_support.c`: Sleeping IT Support simulation
- `ai_researchers.c`: AI Researchers and GPUs simulation with four required strategies
- `test_script.sh`: build and smoke-test script
- `Makefile`: local build helper
- `TASK_DAGILIMI.md`: three-person task split
- `project_report_template.md`: report skeleton

## Build

```bash
make
```

## Test

Run the script from WSL or a Linux shell on Windows:

```bash
bash test_script.sh
```

On this machine, native Windows GCC does not expose `pthread.h` and `semaphore.h`, so WSL/Linux is the intended validation path.

## Team Split

The work is divided so each member owns a clear module and video section. See `TASK_DAGILIMI.md`.
