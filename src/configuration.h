// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stddef.h>
#include <stdio.h>

typedef struct numa_domain {
  size_t num_cpus;
  int *cpu_ids;
  size_t num_nvram;
  char **nvram;
} numa_domain_t;

typedef struct system_topology {
  size_t num_domains;
  numa_domain_t *domains;
} system_topology_t;

void print_configuration(void);

#define TO_GIB(val) (val * 1024ULL * 1024ULL * 1024ULL)

extern numa_domain_t CONST_TOPO;
