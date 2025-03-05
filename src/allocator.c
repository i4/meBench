// SPDX-License-Identifier: GPL-2.0-or-later

#include "allocator.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

#include <numa.h>

#include "configuration.h"
#include "parameters.h"

static void pdie(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
};

void *alloc_memory(size_t cpu_index, int cpu_id) {
  int curr_numa = numa_node_of_cpu((int)cpu_id);
  if (curr_numa == -1) {
    pdie("numa_node_of_cpu");
  }

  // NOTE: Our benchmark currently only supports a maximum of 2 numa nodes.
  // Consequently, the far node is always the current numa node xor'ed with 1.
#if defined(NUMA_DISTANCE_NEAR)
  int target_numa = curr_numa;
#elif defined(NUMA_DISTANCE_FAR)
  int target_numa = curr_numa & 1;
#endif // NUMA_DISTANCE

  size_t size = TO_GIB(MEM_PER_THREAD);

#if defined(MEMTYPE_DRAM)
  return numa_alloc_onnode(size, target_numa);
#elif defined(MEMTYPE_NVRAM)
  numa_domain_t *domain = &CONST_TOPO.domains[target_numa];
  // Allocate memory via mmap from given nvram-dax-devices on a
  // round-robin-basis
  char *nvram = domain->nvram[cpu_index % domain->num_nvram];

  off_t offset = (off_t)((cpu_index / domain->num_nvram) * size);

  int fd = open(nvram, O_RDWR, 0);
  if (fd == -1) {
    pdie("open");
  }
  void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_SHARED_VALIDATE | MAP_SYNC, fd, offset);
  if (mem == MAP_FAILED) {
    pdie("mmap");
  }

  return mem;
#endif // MEMTYPE
}

void free_memory(void *mem) {
  size_t size = TO_GIB(MEM_PER_THREAD);
#if defined(MEMTYPE_DRAM)
  numa_free(mem, size);
#elif defined(MEMTYPE_NVRAM)
  munmap(mem, size);
#endif // MEMTYPE
}
