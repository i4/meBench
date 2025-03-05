#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

bash -n "$(command -v "$0")"
set -x

readonly dst=$1

if ls /sys/devices/system/cpu/cpu0/cpufreq/ > /dev/null
then
	min_freq_khz=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq)
	max_freq_khz=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq)
	old_governor=$(cat cpufreq-governor)
	sudo cpupower frequency-set \
		--min ${min_freq_khz}kHz --max ${max_freq_khz}kHz \
		--governor ${old_governor} &
fi

wait
