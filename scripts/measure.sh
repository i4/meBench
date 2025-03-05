#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

if [ -z "$2" ]; then
	echo "Usage: $0 <benchmark_directory> <result_path>"
	exit 1
fi

readonly run_dir=$1
shift
readonly result=$1

readonly benchmark="${run_dir}/benchmark"

perf stat \
	--output ${run_dir}/perf.out -x , \
	-e power/energy-pkg/ \
	-e power/energy-ram/ \
	-e duration_time \
	${PERF_EVENTS} \
	./${benchmark} > ${result} 2> ${run_dir}/results.stderr
