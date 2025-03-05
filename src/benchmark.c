// SPDX-License-Identifier: GPL-2.0-or-later

#include <emmintrin.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <time.h>

#include "allocator.h"
#include "configuration.h"
#include "parameters.h"

typedef struct thread_info {
  pthread_t id;
  int cpu_id;
  size_t thread_num;
  void *mem;
  int rand_seed;
  uint64_t res;
} thread_info_t;

void pdie(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
};

void fdie(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
};

volatile bool running_flag = true;

#undef ACCESS_TYPE_T
#if ACCESS_SIZE == 1
#define ACCESS_TYPE_T int8_t
#elif ACCESS_SIZE == 2
#define ACCESS_TYPE_T int16_t
#elif ACCESS_SIZE == 4
#define ACCESS_TYPE_T int32_t
#elif ACCESS_SIZE == 8
#define ACCESS_TYPE_T int64_t
#elif ACCESS_SIZE == 16
#define ACCESS_TYPE_T __int128
#endif // ACCESS_SIZE

#if NUM_LOADS > 0
static inline void do_load(void *mem) {
  volatile ACCESS_TYPE_T value = *((ACCESS_TYPE_T *)mem);
  (void)value; // prevent unused warning

#if defined(CLFLUSH)
  _mm_clflush(mem);
#endif // OPERATION_MODE
}
#endif // NUM_LOADS

#if NUM_STORES > 0
static inline void do_store(void *mem, struct drand48_data *buffer) {
  long int random[4];
#if ACCESS_SIZE >= 1
  lrand48_r(buffer, &random[0]);
#endif
#if ACCESS_SIZE >= 4
  lrand48_r(buffer, &random[1]);
#endif
#if ACCESS_SIZE >= 16
  lrand48_r(buffer, &random[2]);
  lrand48_r(buffer, &random[3]);
#endif

  ACCESS_TYPE_T *ptr = (ACCESS_TYPE_T *)mem;
#if ACCESS_SIZE == 1 || ACCESS_SIZE == 2
  *ptr = (ACCESS_TYPE_T)random[0];
#elif ACCESS_SIZE == 4
#if defined(NONTEMPORAL)
  __builtin_ia32_movnti(ptr, (ACCESS_TYPE_T)random[0]);
#else
  *ptr = (ACCESS_TYPE_T)random[0];
#endif // OPERATION_MODE
#elif ACCESS_SIZE == 8
  ACCESS_TYPE_T val = (ACCESS_TYPE_T)random[0] | (ACCESS_TYPE_T)random[1] << 32;
#if defined(NONTEMPORAL)
  __builtin_ia32_movnti64((long long *)ptr, val);
#else
  *ptr = val;
#endif // OPERATION_MODE
#elif ACCESS_SIZE == 16
  ACCESS_TYPE_T val =
      (ACCESS_TYPE_T)random[0] | (ACCESS_TYPE_T)random[1] << 32 |
      (ACCESS_TYPE_T)random[2] << 64 | (ACCESS_TYPE_T)random[3] << 96;
#if defined(NONTEMPORAL)
  // for some reason, ptr and val are in reverse order in comparison to
  // the remaining builtins...
  __builtin_nontemporal_store(val, ptr);
#else
  *ptr = val;
#endif // OPERATION_MODE
#endif // ACCESS_SIZE

#if defined(CLFLUSH)
  _mm_clflush(mem);
#endif // OPERATION_MODE
}
#endif // NUM_STORES

static void do_work(void *mem, struct drand48_data *buffer, size_t work_count) {
#if NUM_LOADS == 0 && NUM_STORES > 0
  do_store(mem, buffer);
#elif NUM_LOADS > 0 && NUM_STORES == 0
  do_load(mem);
#else
  size_t curr_workload = work_count % (NUM_LOADS + NUM_STORES);
  if (curr_workload < NUM_LOADS) {
    do_load(mem);
  } else {
    do_store(mem, buffer);
  }
#endif // NUM_LOADS/NUM_STORES
}

static inline void next_chunk(size_t *index, struct drand48_data *buffer) {
#if defined(SEQUENTIAL_ACCESS)
  *index += CHUNK_SIZE;
#elif defined(RANDOM_ACCESS)
  long int random;
  lrand48_r(buffer, &random);
  // ensure that mem_per_thread is not exceeded and alignment to chunk_size
  // is respected
  *index = ((size_t)random % MEM_PER_THREAD) & ~(CHUNK_SIZE - 1);
#endif // RANDOM_ACCESS
};

void *benchmark_routine(void *arg) {
  thread_info_t *param = (thread_info_t *)arg;

  // printf("ID: %ld\n", param->thread_num);
  // printf("MEM: %p\n", param->mem);
  // printf("RAND_SEED: %d\n", param->rand_seed);

  // configure thread-local random seed
  struct drand48_data rand_buf;
  srand48_r(param->rand_seed, &rand_buf);

  // keep track of number of accesses for scheduling mixed workload and for
  // returning total work performed after benchmark has ended
  size_t work_count = 0;

  while (true) {
    // use (uint8_t *) for pointer arithmetic as it is not undefined for (void*)
    uint8_t *mem = (uint8_t *)param->mem;
    size_t chunk_offset = 0;
#if defined(RANDOM_ACCESS)
    // When performing random access patterns, we need to initialize our offset
    // with an initial value, too.
    next_chunk(&chunk_offset, &rand_buf);
#endif // RANDOM_ACCESS

    for (size_t chunk_index = 0;
         chunk_index < (TO_GIB(MEM_PER_THREAD) / CHUNK_SIZE) && running_flag;
         ++chunk_index) {
      for (size_t access_offset = 0; access_offset < CHUNK_SIZE;
           access_offset += ACCESS_SIZE) {
        do_work((void *)&mem[chunk_offset + access_offset], &rand_buf,
                work_count++);
      }
      next_chunk(&chunk_offset, &rand_buf);
    }
    if (!running_flag)
      break;
  }

  // Return total number of written and read bytes
  param->res = work_count * ACCESS_SIZE;
  return NULL;
}

void benchmark(void) {
  size_t total_cpus = 0;
  for (size_t domain = 0; domain < CONST_TOPO.num_domains; ++domain) {
    total_cpus += CONST_TOPO.domains[domain].num_cpus;
  }

  thread_info_t *t_info = calloc(total_cpus, sizeof(*t_info));
  if (!t_info) {
    pdie("calloc");
  }

  // set random seed to make benchmarks repeatable/deterministic
  srand(0xBAADF00D);
  size_t index = 0;
  for (size_t domain = 0; domain < CONST_TOPO.num_domains; ++domain) {
    for (size_t cpu = 0; cpu < CONST_TOPO.domains[domain].num_cpus; ++cpu) {
      // prepare thread_info-struct
      t_info[index].thread_num = index;
      t_info[index].cpu_id = CONST_TOPO.domains[domain].cpu_ids[cpu];
      t_info[index].mem = alloc_memory(cpu, t_info[index].cpu_id);
      if (!t_info[index].mem) {
        pdie("alloc_memory");
      }
      // provide each thread with a repeatable pseudorandom seed (thanks to
      // srand)
      t_info[index].rand_seed = rand();

      errno = pthread_create(&t_info[index].id, NULL, &benchmark_routine,
                             &t_info[index]);
      if (errno) {
        pdie("pthread_create");
      }

      // pin thread to cpu core
      cpu_set_t cpu_set;
      CPU_ZERO(&cpu_set);
      CPU_SET(t_info[index].cpu_id, &cpu_set);
      errno =
          pthread_setaffinity_np(t_info[index].id, sizeof(cpu_set), &cpu_set);
      if (errno) {
        pdie("pthread_setaffinity_np");
      }
      // increment index for next iteration
      ++index;
    }
  }

  // sleep for duration before signaling all threads to stop their work
  struct timespec ts[2] = {{.tv_sec = DURATION, .tv_nsec = 0},
                           {.tv_sec = 0, .tv_nsec = 0}};
  size_t ts_id = 0;

  int ret;
  while ((ret = nanosleep(&ts[ts_id], &ts[1 - ts_id])) && errno == EINTR) {
    ts_id ^= 0x1;
  }
  if (ret) {
    pdie("nanosleep");
  }
  running_flag = false;

  for (size_t i = 0; i < total_cpus; ++i) {
    errno = pthread_join(t_info[i].id, NULL);
    if (errno) {
      pdie("pthread_join");
    }
  }

  printf("thread_id;cpu_id;bytes_accessed\n");
  for (int i = 0; i < total_cpus; ++i) {
    printf("%ld;%d;%ld\n", t_info[i].thread_num, t_info[i].cpu_id,
           t_info[i].res);
    free_memory(t_info[i].mem);
  }

  free(t_info);
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    print_configuration();
    exit(EXIT_SUCCESS);
  }
  benchmark();
}
