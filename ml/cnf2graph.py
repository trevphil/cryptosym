import dgl
from pathlib import Path
from multiprocessing import Pool, cpu_count
from typing import Tuple

from pysat.formula import CNF as PyCNF
from dgl.data.utils import save_graphs


def convert_cnf_to_graph(args: Tuple[int, Path, Path]) -> None:
    sample_idx, filein, fileout = args

    if fileout.exists():
        return  # Exit early if this file already exists

    cnf = PyCNF(from_file=str(filein))

    lit2node = dict()

    # node 0 = FALSE
    # node 1 = TRUE
    # node 2 = BASE
    edges = [(0, 1), (0, 2), (1, 2)]
    num_nodes = 3

    def _get_node_index(literal: int, nn: int) -> Tuple[int, int]:
        node_index = lit2node.get(literal, None)
        if node_index is None:
            node_index = nn
            lit2node[literal] = node_index
            nn += 1
        return node_index, nn

    for clause in cnf.clauses:
        assert len(clause) > 1

        node_indices = []
        for lit in clause:
            ni, num_nodes = _get_node_index(lit, num_nodes)
            node_indices.append(ni)

        node1 = node_indices[0]
        for node2 in node_indices[1:]:
            edges.append((node1, num_nodes))
            edges.append((node2, num_nodes + 1))
            edges.append((num_nodes, num_nodes + 1))
            edges.append((num_nodes, num_nodes + 2))
            edges.append((num_nodes + 1, num_nodes + 2))
            node1 = num_nodes + 2
            num_nodes += 3
        or_of_literals = node1  # Force this to be TRUE
        edges.append((or_of_literals, 0))
        edges.append((or_of_literals, 2))

    for lit, node_a in lit2node.items():
        # Force literals to be TRUE or FALSE
        edges.append((node_a, 2))
        if -lit in lit2node:
            # Force negated lit to be opposite of lit
            node_b = lit2node[-lit]
            edges.append((node_b, 2))
            edges.append((node_a, node_b))

    src, dst = zip(*edges)
    g = dgl.graph((src, dst), num_nodes=num_nodes)
    g = dgl.to_simple(g)  # Remove parallel edges
    g = dgl.to_bidirected(g)  # Make bidirectional <-->

    if sample_idx % 100 == 0:
        nn = g.num_nodes()
        ne = g.num_edges()
        print(f"Converted sample {sample_idx}:")
        print(f"  nn={nn}, ne={ne}")

    assert g.is_homogeneous, "Graph should be homogeneous"
    save_graphs(str(fileout), [g])


if __name__ == "__main__":
    cnf_base = Path("./cnf_files")
    if not cnf_base.is_dir():
        raise NotADirectoryError(f"{cnf_base} - not a directory")

    graph_base = Path("./graph_files")
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

        arg_list = []
        for i, fin in enumerate(files):
            fout_name = fin.with_suffix(".bin").name
            fout = graph_base / split / fout_name
            arg_list.append((i, fin, fout))

        with Pool(cpu_count()) as p:
            p.map(convert_cnf_to_graph, arg_list)
