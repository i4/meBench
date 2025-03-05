// SPDX-License-Identifier: GPL-2.0-or-later

#define MEM_PER_THREAD (2ul)
#define MEMTYPE_DRAM 1
#define NUMA_DISTANCE_NEAR 1
#define NUM_LOADS (1ul)
#define NUM_STORES (0ul)
#define NORMAL 1
#define SEQUENTIAL_ACCESS 1
#define ACCESS_SIZE (1ul)
#define CHUNK_SIZE (1ul)
#define DURATION (5ul)
#define CONST_TOPO (const_topo)
#define NUM_DOMAINS (2ul)
static int const_dom_cpus_0[] = {0};
static char *const_dom_nvram_0[] = {"/dev/dax0.0"};
static int const_dom_cpus_1[] = {};
static char *const_dom_nvram_1[] = {"/dev/dax1.1"};
static numa_domain_t const_doms[] = {
    {
        .num_cpus = 1,
        .cpu_ids = const_dom_cpus_0,
        .num_nvram = 1,
        .nvram = const_dom_nvram_0,
    },
    {
        .num_cpus = 0,
        .cpu_ids = const_dom_cpus_1,
        .num_nvram = 1,
        .nvram = const_dom_nvram_1,
    },
};
static system_topology_t const_topo = {
    .num_domains = NUM_DOMAINS,
    .domains = const_doms,
};
