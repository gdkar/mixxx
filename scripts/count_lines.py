#! /usr/bin/env python3

import pathlib
import itertools
import sys
import glob
import argparse

parser = argparse.ArgumentParser(description = "Count lines recursively")
parser.add_argument("-v","--verbose", help="verbose output",action="store_true")
parser.add_argument("-d","--dir","--directory",help="root of tree to search",default=pathlib.Path('.'),type=pathlib.Path)
parser.add_argument("-g","--glob",help="globs of files to count",nargs="+",default=["*","**/*"])
parser.add_argument("-x","--exclude",help="globs of files to exclude",nargs="+",default=[])
args = parser.parse_args()
verbose = args.verbose
files = itertools.chain(*(filter(lambda x: x.is_file(),args.dir.glob(g)) for g in args.glob))
if args.exclude: files = filter(lambda x: not any(x.match(e) for e in args.exclude),files)
count = 0
for _file in files:
    try:
        with _file.open(mode='r',encoding='ascii') as f: num = len(f.readlines())
        if verbose:
            print("{}: {}".format(_file.as_posix(),num))
        count += num
    except Exception as e:
        if verbose:
            print("{}: ERROR {}".format(_file.as_posix(),e))
print("Total: {}".format(count))
