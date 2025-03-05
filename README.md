## meBench (Memory Energy Benchmark)

meBench is a simple, configurable workload to allow isolated benchmarking of different interactions with memory.

## Required Packages
 - clang
 - gnumake
 - libnuma
 - perf

## Usage

A dummy workload for development can be generated using `make dummy_params`.
With it present, `make build` can be used to build the respective benchmark and `make run` to run the actual benchmark.

In addition, meBench supports batching via `make generate_params` which can respectively be built using `make build` and executed using `make run_all`.

## Overview

`src`:
 - `src/benchmark.c`: Contains the actual benchmark workload.
 - `src/allocator.[ch]`: Contains memory allocation routines.
 - `src/configuration.ch: Contains configuration validation and helpers.

`scripts`:
 - `scripts/for\_all.py`: Simple enumerator to allow compilation/execution of all previously created workload configurations.
 - `scripts/measure.sh`: Wrapper for workload execution to start/stop measurements (perf, etc.)
 - `scripts/start-benchmarkk-runtime.sh`: Script to prepare system for benchmarking
 - `scripts/end-benchmarkk-runtime.sh`: Script to restore system after finishing benchmark runs

## License

The project is licensed under GNU General Public License v2.0 or later.
