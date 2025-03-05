#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import argparse
import subprocess
import re


def run_local(cmd, verbose=True):
    if verbose:
        print(cmd)
    res = subprocess.run(cmd, check=True, shell=True, capture_output=True)
    if res.returncode != 0:
        print(res.stdout.decode('utf-8'))
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('WORKLOAD', choices={'build', 'run'}, help='makefile target to be executed for all workloads in OBJDIR')
    parser.add_argument('OBJDIR', nargs='?', default='build', help='target parent directory for all workloads')
    args = parser.parse_args()
    natsort = lambda s: [int(t) if t.isdigit() else t.lower() for t in re.split(r'(\d+)', str(s))]
    if args.WORKLOAD == 'build':
        paths = [x for x in Path(args.OBJDIR).glob('**/parameters.h')]
    else:
        paths = [x for x in Path(args.OBJDIR).glob('**/benchmark')]
    for i, path in enumerate(sorted(paths, key=natsort)):
        print(f'[{i+1}/{len(paths)}]')
        build_path = path.parent

        run_local(f'make {args.WORKLOAD} OBJDIR={build_path}')

if __name__ == "__main__":
    main()
