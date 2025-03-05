// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stddef.h>

void *alloc_memory(size_t cpu_index, int cpu_id);
void free_memory(void *mem);
