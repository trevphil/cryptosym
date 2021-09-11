import os
import torch
import numpy as np
from pathlib import Path
from multiprocessing import Pool, cpu_count

from pysat.formula import CNF as PySatCNF
from pysat.solvers import Glucose4
from dgl.data.utils import save_graphs

from logic.mis import MaximumIndependentSet


def recursively_find_cnf(base_dir):
    return [
        os.path.join(dp, f) for dp, dn, filenames in os.walk(base_dir)
        for f in filenames if os.path.splitext(f)[1] == '.cnf'
    ]


def write_graph(args):
    filein, fileout = args

    if os.path.exists(str(fileout)):
        return  # Exit early if this file already exists

    max_solutions = 256
    cnf = PySatCNF(from_file=str(filein))

    with Glucose4() as solver:
        solver.append_formula(cnf.clauses)
        assert solver.solve(), 'All problems should be SAT'

        solutions = []
        for i, m in enumerate(solver.enum_models()):
            if i >= max_solutions:
                return  # Too many solutions, skip this example!
            solutions.append(m)

    num_sol = len(solutions)
    solutions = np.array(solutions, dtype=int).T
    solutions[solutions < 0] = 0
    solutions[solutions > 0] = 1
    solutions = torch.tensor(solutions, dtype=torch.uint8)

    mis = MaximumIndependentSet(cnf=cnf)
    g = mis.g
    label = torch.zeros(g.num_nodes(), num_sol, dtype=torch.uint8)

    for sol_idx in range(num_sol):
        a_sol = mis.cnf_to_mis_solution(solutions[:, sol_idx])
        label[:, sol_idx] = a_sol

    g.ndata['label'] = label
    g.ndata['node2lit'] = mis.node_index_to_lit
    g.ndata['node2clause'] = mis.node_index_to_clause
    info = torch.zeros(g.num_nodes(), dtype=int)
    info[0] = len(cnf.clauses)
    info[1] = num_sol
    g.ndata['info'] = info

    assert g.is_homogeneous, 'Graph should be homogeneous'
    save_graphs(str(fileout), [g])


if __name__ == '__main__':
    base_path = './graph_dataset'
    train = ('train', recursively_find_cnf('./cnf_dataset/train'))
    val = ('val', recursively_find_cnf('./cnf_dataset/val'))
    test = ('test', recursively_find_cnf('./cnf_dataset/test'))
    print(f'Dataset: train({len(train[1])}), val({len(val[1])}), test({len(test[1])})')

    for split, files in [train, val, test]:
        print(f'Starting split: {split}')

        args = []
        for i, f in enumerate(files):
            fname = f.split('/')[-1]
            fname = fname[:fname.rfind('.')] + ('_%05d.bin' % i)
            fileout = Path(base_path) / Path(split) / Path(fname)
            args.append((f, str(fileout)))

        with Pool(cpu_count()) as p:
            p.map(write_graph, args)
