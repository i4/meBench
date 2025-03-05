# SPDX-License-Identifier: GPL-2.0-or-later

VERBOSE ?= @
SRCDIR := src
OBJDIR := build
ARGS :=

CC := clang
CFLAGS := -std=c2x -Wconversion -Wall -Werror -D_GNU_SOURCE -g $(shell pkg-config --cflags numa) -I $(OBJDIR)
LDFLAGS := $(shell pkg-config --libs numa)

.PHONY: all build build_all generate_params run run_all clean

TARGET := $(OBJDIR)/benchmark
# parameters for each workload can be generated using 'generate_params'-target
PARAMS := $(OBJDIR)/parameters.h

all: $(TARGET)

dummy_params:
	@mkdir -p $(dir $(PARAMS))
	cp $(SRCDIR)/example-parameters.h $(PARAMS)

generate_params:
	@mkdir -p $(OBJDIR)
	./scripts/generate_params.py "$(OBJDIR)"

build_all:
	./scripts/for_all.py build "$(OBJDIR)"

build: $(TARGET)

run_all:
	./scripts/for_all.py run "$(OBJDIR)"

run: $(OBJDIR)/results.csv

$(OBJDIR)/results.csv: $(OBJDIR)/benchmark
	scripts/measure.sh "$(OBJDIR)" "$@"

$(OBJDIR)/benchmark: $(OBJDIR)/benchmark.o $(OBJDIR)/configuration.o $(OBJDIR)/allocator.o $(PARAMS)
	@echo "LD    $@"
	$(VERBOSE) $(CC) $(CFLAGS) $(filter %.o,$^) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(PARAMS)
	@echo "C     $@"
	@mkdir -p $(dir $@)
	$(VERBOSE) $(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "RM    $(OBJDIR)"
	$(VERBOSE) rm -rf $(OBJDIR)
