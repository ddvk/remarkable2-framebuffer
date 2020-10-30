#!/usr/bin/env python3
from __future__ import print_function
import argparse
import copy
import os
import sys
from collections import defaultdict

DEBUG = os.getenv("DEBUG", True)
DEBUG_GRAPH=False

def gather_includes(file, includes, parent=None, base_dir=None):
    file_path = os.path.normpath(os.path.join(base_dir or "", file))

    if file_path == parent:
        return


    if file_path in includes:
        if parent:
            includes[parent].add(file_path)
        return

    if parent:
        includes[parent].add(file_path)

    dirname = os.path.dirname(file)
    fname = os.path.basename(file)

    if base_dir == None:
        base_dir = dirname
    else:
        base_dir = os.path.join(base_dir, dirname)

    if not os.path.exists(file_path):
        print("missing file:", file, file=sys.stderr)
        return

    file = os.path.join(base_dir, fname)

    with open(file_path) as f:
        lines = f.readlines()

    for line in lines:
        cline = line.strip()
        if cline.startswith("#include"):
            tokens = cline.split()
            include = tokens[1]
            if include[0] != '"':
                continue
            include = include.strip('"')

            fname, ext = os.path.splitext(include)
            gather_includes(include, includes, parent=file_path, base_dir=base_dir)

    return includes


def gather_files(files):
    ret = defaultdict(set)
    original_dir = os.getcwd()
    for file in files:
        if file == "-":
            ret[file] = []
            continue

        if not os.path.exists(file):
            continue
        file_dir, file = os.path.split(file)
        gather_includes(file, ret, parent=None, base_dir=file_dir)

    os.chdir(original_dir)

    full_graph = defaultdict(set)
    for node, neighbors in ret.items():
        for neighbor in neighbors:
            if not neighbor in full_graph:
                full_graph[neighbor] = set()
        full_graph[node].update(neighbors)

    return full_graph


def get_parser():
    parser = argparse.ArgumentParser(
        description="Take multiple .h and .cpp files and output a single .h"
    )
    parser.add_argument(
        "files",
        metavar="files",
        type=str,
        nargs="+",
        help=".h and .cpp files to combine",
    )
    return parser


def n2_top_sort(graph):
    ret = []
    while graph:
        batch = set()
        for f, deps in graph.items():
            if not deps:
                batch.add(f)
        for f in graph.keys():
            graph[f] -= batch

        ret.extend(list(sorted(batch)))
        for f in batch:
            del graph[f]
        if not batch and graph:
            print("found cyclic dependency", graph, file=sys.stderr)
            exit()
    return list(ret)


def concat_files(files, outfile, graph):
    outfile.write("#define __SH_BUILD");
    for file in files:
        if not os.path.exists(file):
            print("MISSING FILE!", file, file=sys.stderr)
            continue

        with open(file, "r") as infile:
            if DEBUG:
                outfile.write("\n/* FILE: %s */\n" % file)
                deps = graph[file]
                outfile.write("\n/* REQUIRES:")
                for f in deps:
                    outfile.write("\n%s" % f)
                outfile.write(" */\n")

            for line in infile.readlines():
                if not (line.startswith("#include \"")):
                    outfile.write(line)
        outfile.write("\n")


def print_graph(graph):
    for node in graph:
        print(node, file=sys.stderr)
        for n in graph[node]:
            print("  ", n, file=sys.stderr)
        print("", file=sys.stderr)

def compile(files, into=None):
    graph = gather_files(files)
    orig_graph = copy.deepcopy(graph)

    if DEBUG_GRAPH:
        print_graph(graph)

    ordered = n2_top_sort(graph)
    if not into.endswith(".h") and not into.endswith(".hpp"):
        into = "%s.h" % (into)
    print("generating single header", into, file=sys.stderr)
    with open(into, "w") as output:
        concat_files(ordered, output, orig_graph)


if __name__ == "__main__":
    parser = get_parser()
    args, unknown = parser.parse_known_args()
    if not args.files:
        parser.print_help()
    else:
        if not os.path.exists("dist"):
            os.makedirs("dist")
        compile(args.files, "dist/librm2fb.hpp")
