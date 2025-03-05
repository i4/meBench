#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import argparse
import math
import subprocess
import sys

def stringify(val):
    if type(val) == list:
        return [stringify(x) for x in val]
    elif type(val) == tuple:
        return tuple(stringify(list(val)))
    else:
        return str(val)

def confirm_prompt(prompt):
    reply = None
    while reply not in ('', 'y', 'yes', 'YES', 'n', 'no', 'NO'):
        reply = input(f'{prompt} ')
    return (reply in ('', 'y', 'yes', 'YES'))

class Domain:
    def __init__(self, cpus, nvrams):
        self.cpus = stringify(cpus)
        self.nvrams = nvrams

    def write_c_def(self, index, file):
        print(f'static int const_dom_cpus_{index}[] = {{ {", ".join(self.cpus)} }};', file=file)
        nvrams = [ f'\"{x}\"' for x in self.nvrams ]
        print(f'static char *const_dom_nvram_{index}[] = {{ {", ".join(nvrams)} }};', file=file)

class Topology:
    @staticmethod
    def create(num_cpus, num_threads):
        domains = []
        for i in range(num_cpus):
            cores = [x for x in range(num_threads) if x % num_cpus == i]
            domains.append(Domain(cores, [f'/dev/dax{i}.0']))
        if num_cpus == 1:
            domains.append(Domain([], ['/dev/dax1.0']))
        return Topology(f'{num_cpus}c{num_threads}t', domains)

    def __init__(self, name, domains: Domain):
        self.name = name
        self.domains = domains

    def get_num_threads(self):
        return sum([len(domain.cpus) for domain in self.domains])

    def write_c_def(self, file):
        print(f'#define CONST_TOPO (const_topo)', file=file)
        print(f'#define NUM_DOMAINS ({len(self.domains)}ul)', file=file)

        for i, dom in enumerate(self.domains):
            dom.write_c_def(i, file)

        print(f'static numa_domain_t const_doms[] = {{', file=file)
        for i, dom in enumerate(self.domains):
            print(f'  {{', file=file)
            print(f'    .num_cpus = {len(dom.cpus)},', file=file)
            print(f'    .cpu_ids = const_dom_cpus_{i},', file=file)
            print(f'    .num_nvram = {len(dom.nvrams)},', file=file)
            print(f'    .nvram = const_dom_nvram_{i},', file=file)
            print(f'  }},', file=file)
        print(f'}};', file=file)

        print(f'static system_topology_t const_topo = {{', file=file)
        print(f'  .num_domains = NUM_DOMAINS,', file=file)
        print(f'  .domains = const_doms,', file=file)
        print(f'}};', file=file)

class Workload:
    def __init__(self, topology, mem_per_thread, duration, memtype, workload_ratio,
                 dist, opmode, pattern, chunk_size, access_size):
        self.topology = topology
        self.mem_per_thread = mem_per_thread
        self.duration = duration
        self.memtype = memtype
        self.workload_ratio = workload_ratio
        self.dist = dist
        self.opmode = opmode
        self.pattern = pattern
        self.chunk_size = chunk_size
        self.access_size = access_size

    def name(self):
        workload_str = '-'.join(stringify(self.workload_ratio))
        return f'{self.topology.name}_workload{workload_str}_{self.mem_per_thread}GiB_{self.memtype}_{self.dist}_{self.opmode}_{self.pattern}_ac{self.access_size}-{self.chunk_size}'

    def write_c_def(self, file):
        print(f'#define MEM_PER_THREAD ({self.mem_per_thread}ul)', file=file)
        print(f'#define DURATION ({self.duration}ul)', file=file)
        print(f'#define MEMTYPE_{self.memtype.upper()} 1', file=file)
        print(f'#define NUMA_DISTANCE_{self.dist.upper()} 1', file=file)
        print(f'#define NUM_LOADS ({self.workload_ratio[0]}ul)', file=file)
        print(f'#define NUM_STORES ({self.workload_ratio[1]}ul)', file=file)
        print(f'#define {self.opmode.upper()} 1', file=file)
        print(f'#define {self.pattern.upper()}_ACCESS 1', file=file)
        print(f'#define ACCESS_SIZE ({self.access_size}ul)', file=file)
        print(f'#define CHUNK_SIZE ({self.chunk_size}ul)', file=file)
        self.topology.write_c_def(file)

TOPOLOGIES = [
    Topology.create(1, 1),
    Topology.create(2, 8),
    Topology.create(2, 16),
    Topology.create(2, 32),
    Topology.create(2, 112),
]

DURATION = 80
MAX_MEMORY = 128 # in GiB
MEM_TYPES = [ "dram", "nvram" ]
WORKLOADS = [ (1, 0), (0, 1), (1, 1) ]
NUMA_DISTANCES = [ "near", "far" ]
OPERATION_MODES = [ "normal", "clflush", "nontemporal" ]
ACCESS_PATTERNS = [ "sequential", "random" ]
ACCESS_SIZES = [ 1, 2, 4, 8, 16 ]
CHUNK_SIZES = [ 1, 2, 4, 8, 16 ]

def generate_workloads(args):
    for topology in TOPOLOGIES:
        if args.prompt and not confirm_prompt(f'Generate benchmarks for {topology.name}?'):
            continue

        mem_per_thread = int(MAX_MEMORY / topology.get_num_threads())
        for workload in WORKLOADS:
            for pattern in ACCESS_PATTERNS:
                for memtype in MEM_TYPES:
                    for dist in NUMA_DISTANCES:
                        for opmode in OPERATION_MODES:
                            for chunk_size in CHUNK_SIZES:
                                for access_size in [ x for x in ACCESS_SIZES if x <= chunk_size ]:
                                    yield Workload(topology, mem_per_thread, DURATION, memtype, workload, dist, opmode, pattern, chunk_size, access_size)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('OBJDIR', nargs='?', default='build', help='target parent directory for all workloads to be generated')
    parser.add_argument('-p', '--prompt', action='store_true', help='prompt for every topology during workload generation whether to include it')
    args = parser.parse_args()

    workloads = [x for x in generate_workloads(args)]
    print(f'Total workloads: {len(workloads)}')
    for i, w in enumerate(workloads):
        print(f'[{i+1}/{len(workloads)}]')
        build_path = f'{args.OBJDIR}/{w.name()}'
        Path(build_path).mkdir(parents=True, exist_ok=True)
        with open(f'{build_path}/parameters.h', 'w') as f:
            w.write_c_def(f)

if __name__ == "__main__":
    main()
