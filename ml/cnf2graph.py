import dgl
import torch
from pathlib import Path
from multiprocessing import Pool, cpu_count
from typing import Tuple

from pysat.formula import CNF as PyCNF
from dgl.data.utils import save_graphs


def get_nv_nc(cnf_file: Path) -> Tuple[int, int]:
    with open(cnf_file, "r") as f:
        for line in f:
            if line.startswith("p cnf"):
                parts = line.strip().split(" ")
                nv = int(parts[2])
                nc = int(parts[3])
                return nv, nc
    return None


def write_graph(args: Tuple[int, Path, Path]) -> None:
    sample_idx, filein, fileout = args

    if fileout.exists():
        return  # Exit early if this file already exists

    cnf = PyCNF(from_file=str(filein))
    nv, nc = get_nv_nc(filein)
    lit2node = dict()

    # node 0 = false
    # node 1 = true
    # node 2 = base
    edges = [(0, 1), (0, 2), (1, 2)]
    node_idx = 3
    for lit in range(1, nv + 1):
        lit2node[lit] = node_idx
        node_idx += 1
        lit2node[-lit] = node_idx
        node_idx += 1

    for clause in cnf.clauses:
        assert len(clause) > 1
        node1 = lit2node[clause[0]]
        for i in range(1, len(clause)):
            node2 = lit2node[clause[i]]
            edges.append((node1, node_idx))
            edges.append((node2, node_idx + 1))
            edges.append((node_idx, node_idx + 1))
            edges.append((node_idx, node_idx + 2))
            edges.append((node_idx + 1, node_idx + 2))
            node1 = node_idx + 2
            node_idx += 3
        edges.append((node1, 0))
        edges.append((node1, 2))

    src, dst = zip(*edges)
    g = dgl.graph((src, dst), num_nodes=node_idx, idtype=torch.int64)
    g = dgl.to_simple(g)  # Remove parallel edges
    g = dgl.to_bidirected(g)  # Make bidirectional <-->

    if sample_idx % 100 == 0:
        nn = g.num_nodes()
        ne = g.num_edges()
        print(f"Converted sample {sample_idx}:")
        print(f"  nv={nv}, nc={nc}, nn={nn}, ne={ne}")

    assert g.is_homogeneous, "Graph should be homogeneous"
    save_graphs(str(fileout), [g])


if __name__ == "__main__":
    graph_base = Path("./graph_files")
    cnf_base = Path("./cnf_files")
    train = ("train", list((cnf_base / "train").rglob("*.cnf")))
    val = ("val", list((cnf_base / "val").rglob("*.cnf")))
    test = ("test", list((cnf_base / "test").rglob("*.cnf")))

    print("Dataset:")
    print(f"\tN_TRAIN={len(train[1])}")
    print(f"\tN_VAL={len(val[1])}")
    print(f"\tN_TEST={len(test[1])}")

    for split, files in (train, val, test):
        print(f"Starting split: {split}")

        (graph_base / split).mkdir(parents=True, exist_ok=True)

        args = []
        for i, fin in enumerate(files):
            fout_name = fin.with_suffix(".bin").name
            fout = graph_base / split / fout_name
            args.append((i, fin, fout))

        with Pool(cpu_count()) as p:
            p.map(write_graph, args)
