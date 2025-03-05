#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

bash -n "$(command -v "$0")"
set -x

readonly dst=$1
mkdir -p $dst

if ls /sys/devices/system/cpu/cpu0/cpufreq/ > /dev/null
then
	# Store current cpu mode for later restoration
	cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor > cpufreq-governor

	max_freq_khz=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq)
	sudo cpupower frequency-set \
		--min ${max_freq_khz}kHz --max ${max_freq_khz}kHz \
		--governor performance
fi

mkdir -p $dst/sysctl.d
sudo sysctl --version > $dst/sysctl.d/version &
sudo sysctl --all > $dst/sysctl.d/all &

sudo cpupower frequency-info > ${dst}/cpupower-frequency-info &
sudo cpupower --version > ${dst}/cpupower_version &

sudo perf --version > ${dst}/perf_version &

wait
