import torch
import dgl
from collections import defaultdict

from logic.gate import gate_type_to_str, GateType


def problem_to_ggnn_graph(problem, observed):
    num_nodes = problem.num_vars
    src, dst, edge_types = [], [], []

    edge_dict = {t: i for i, t in enumerate(GateType)}

    for gate in problem.gates:
        assert gate.output > 0
        node_to = gate.output - 1
        for inp in gate.inputs:
            # TODO
            # assert inp > 0, f'Not supported yet: inverted inputs (inp={inp})'
            node_from = abs(inp) - 1
            src.append(node_from)
            dst.append(node_to)
            edge_types.append(edge_dict[gate.t])

    g = dgl.graph((src, dst), num_nodes=num_nodes, idtype=torch.int32)

    g.edata['type'] = torch.tensor(edge_types, dtype=torch.int32)

    feat = torch.normal(0, 0.1, (num_nodes, 4))
    for var, val in observed.items():
        assert var > 0
        feat[abs(var) - 1, 0] = 1.0 if val else -1.0
    for idx in range(num_nodes):
        feat[idx, 1] = problem.bit_depths[idx + 1]
    feat[:, 1] = 1.0 - (feat[:, 1] / torch.max(feat[:, 1]))
    feat[:, 2] = g.in_degrees()
    feat[:, 2] /= torch.max(feat[:, 2])
    feat[:, 3] = g.out_degrees()
    feat[:, 3] /= torch.max(feat[:, 3])
    g.ndata['feat'] = feat

    return g
