// SPDX-License-Identifier: GPL-2.0-or-later

// First include external parameters and verify them statically afterwards

#include "configuration.h"

#include "parameters.h"

#if !defined(MEM_PER_THREAD)
#error MEM_PER_THREAD not defined
#endif

#if !defined(MEMTYPE_DRAM) && !defined(MEMTYPE_NVRAM)
#error MEMTYPE not defined
#elif defined(MEMTYPE_DRAM) && defined(MEMTYPE_NVRAM)
#error Too many MEMTYPE-variants defined
#endif

#if !defined(NUMA_DISTANCE_NEAR) && !defined(NUMA_DISTANCE_FAR)
#error NUMA_DISTANCE not defined
#elif defined(NUMA_DISTANCE_NEAR) && defined(NUMA_DISTANCE_FAR)
#error Too many NUMA_DISTANCE-variants defined
#endif

#if !defined(NUM_LOADS) || !defined(NUM_STORES)
#error NUM_LOADS or NUM_STORES not defined
#elif NUM_LOADS <= 0 && NUM_STORES <= 0
#error NUM_LOADS and NUM_STORES must be positive and at least one must be greater than 0
#endif

#if !defined(NORMAL) && !defined(CLFLUSH) && !defined(NONTEMPORAL)
#error OPERATION_MODE not defined
#endif

#if !defined(SEQUENTIAL_ACCESS) && !defined(RANDOM_ACCESS)
#error ACCESS_PATTERN not defined
#elif defined(SEQUENTIAL_ACCESS) && defined(RANDOM_ACCESS)
#error Too many ACCESS_PATTERN-variants defined
#endif

#if !defined(ACCESS_SIZE)
#error ACCESS_SIZE not defined
#elif ACCESS_SIZE != 1 && ACCESS_SIZE != 2 && ACCESS_SIZE != 4 &&              \
    ACCESS_SIZE != 8 && ACCESS_SIZE != 16
#error Invalid ACCESS_SIZE defined
#endif

#if !defined(CHUNK_SIZE)
#error CHUNK_SIZE not defined
#elif CHUNK_SIZE != 1 && CHUNK_SIZE != 2 && CHUNK_SIZE != 4 &&                 \
    CHUNK_SIZE != 8 && CHUNK_SIZE != 16
#error Invalid CHUNK_SIZE defined
#endif

#if !defined(DURATION)
#error DURATION not defined
#endif

#if !defined(CONST_TOPO)
#error CONST_TOPO not defined
#endif

#if !defined(NUM_DOMAINS)
#error NUM_DOMAINS not defined
#elif NUM_DOMAINS <= 0 || NUM_DOMAINS > 2
#error Invalid NUM_DOMAINS defined
#endif

#if defined(NUMA_DISTANCE_FAR) && NUM_DOMAINS < 2
#error Set NUM_DOMAINS not compatible with NUMA_DISTANCE_FAR
#endif

static void print_system_topology(struct system_topology *topo) {

  printf("system_topology = [\n");
  for (size_t dom = 0; dom < topo->num_domains; ++dom) {
    printf("    dom[%ld] = {\n", dom);
    printf("        cpu_ids = [ ");
    for (size_t cpu = 0; cpu < topo->domains[dom].num_cpus; ++cpu) {
      printf("%d ", topo->domains[dom].cpu_ids[cpu]);
    }
    printf("],\n");
    printf("        nvram = [\n");
    for (size_t nvram = 0; nvram < topo->domains[dom].num_nvram; ++nvram) {
      printf("            \"%s\",\n", topo->domains[dom].nvram[nvram]);
    }
    printf("        ],\n");
    printf("    },\n");
    printf("]\n");
  }
}

void print_configuration(void) {
#if defined(MEMTYPE_DRAM)
  char *memtype = "dram";
#elif defined(MEMTYPE_NVRAM)
  char *memtype = "nvram";
#endif
  printf("MEM_TYPE = %s\n", memtype);

#if defined(NUMA_DISTANCE_NEAR)
  char *dist = "near";
#elif defined(NUMA_DISTANCE_FAR)
  char *dist = "far";
#endif
  printf("NUMA_DISTANCE = %s\n", dist);

  printf("OPERATION_MODE =");
#if defined(NORMAL)
  printf(" NORMAL");
#endif
#if defined(CLFLUSH)
  printf(" CLFLUSH");
#endif
#if defined(NONTERMPORAL)
  printf(" NONTEMPORAL");
#endif
  printf("\n");

#if defined(SEQUENTIAL_ACCESS)
  char *pattern = "sequential";
#elif defined(RANDOM_ACCESS)
  char *pattern = "random";
#endif
  printf("ACCESS_PATTERN = %s\n", pattern);

  struct {
    char *name;
    int val;
  } params[] = {
      {"MEM_PER_THREAD", MEM_PER_THREAD}, {"NUM_LOADS", NUM_LOADS},
      {"NUM_STORES", NUM_STORES},         {"ACCESS_SIZE", ACCESS_SIZE},
      {"CHUNK_SIZE", CHUNK_SIZE},         {"DURATION", DURATION},
  };

  for (unsigned i = 0; i < sizeof(params) / sizeof(*params); i++) {
    printf("%s = %d\n", params[i].name, params[i].val);
  }
  printf("\n");

  print_system_topology(&CONST_TOPO);
}
