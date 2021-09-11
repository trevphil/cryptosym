import torch
import dgl
from copy import deepcopy
from collections import defaultdict

from logic.cnf import CNF


class MaximumIndependentSet(object):
    def __init__(self, cnf=None, g=None):
        self.g = None
        self.node_index_to_lit = None
        self.node_index_to_clause = None
        self.lit_to_node_indices = defaultdict(list)

        if cnf is not None:
            assert g is None, 'Cannot init MIS with CNF and graph'
            self.init_from_cnf(cnf)
        elif g is not None:
            assert cnf is None, 'Cannot init MIS with CNF and graph'
            self.init_from_graph(g)
        else:
            assert False, 'Must init MIS with CNF or graph'

        max_clause_idx = self.node_index_to_clause.max().detach().item()
        max_lit = self.node_index_to_lit.abs().max().detach().item()
        self.expected_num_clauses = int(max_clause_idx) + 1
        self.expected_num_vars = int(max_lit)
        self.pos_index = (self.node_index_to_lit > 0)
        self.neg_index = (self.node_index_to_lit < 0)

    def init_from_cnf(self, cnf):
        num_nodes = sum(len(clause) for clause in cnf.clauses)
        src, dst = [], []
        node_index = 0
        self.node_index_to_lit = torch.zeros(num_nodes, dtype=int)
        self.node_index_to_clause = torch.zeros(num_nodes, dtype=int)

        for clause_idx, clause in enumerate(cnf.clauses):
            assert clause is not None and len(clause) > 1

            num_lits = len(clause)
            clause = list(clause)
            nodes = list(range(node_index, node_index + num_lits))

            for i in range(num_lits):
                lit = clause[i]
                node = nodes[i]
                self.node_index_to_lit[node] = lit
                self.node_index_to_clause[node] = clause_idx
                self.lit_to_node_indices[lit].append(node)

            for i in range(num_lits - 1):
                for j in range(i + 1, num_lits):
                    src.append(node_index + i)
                    dst.append(node_index + j)

            node_index += num_lits

        for lit, set_a in self.lit_to_node_indices.items():
            if (-lit) not in self.lit_to_node_indices:
                continue
            set_b = self.lit_to_node_indices[-lit]
            for a in set_a:
                for b in set_b:
                    src.append(a)
                    dst.append(b)

        assert node_index == num_nodes, f'{node_index} != {num_nodes}'
        g = dgl.graph((src, dst), num_nodes=num_nodes, idtype=torch.int64)
        g = dgl.to_simple(g)  # Remove parallel edges
        g = dgl.to_bidirected(g)  # Make bidirectional <-->
        self.g = g

    def init_from_graph(self, g):
        assert g.is_homogeneous, 'MIS graph should be homogeneous'
        self.g = g
        self.node_index_to_lit = g.ndata['node2lit']
        self.node_index_to_clause = g.ndata['node2clause']

        n = g.num_nodes()
        i = 0
        while i < n:
            c = self.node_index_to_clause[i]
            while i < n and self.node_index_to_clause[i] == c:
                lit = self.node_index_to_lit[i].detach().item()
                self.lit_to_node_indices[lit].append(i)
                i += 1

    @property
    def num_nodes(self):
        return self.g.num_nodes()

    @property
    def num_edges(self):
        return self.g.num_edges()

    def cnf_to_mis_solution(self, bits):
        n = self.num_nodes

        # Label each node according to truth value of corresponding literal
        label = torch.zeros(n, dtype=bits.dtype, device=bits.device)
        label[self.pos_index] = torch.take(bits,
                self.node_index_to_lit[self.pos_index] - 1)
        label[self.neg_index] = 1 - torch.take(bits,
                -self.node_index_to_lit[self.neg_index] - 1)

        # Zero-out nodes after first non-zero node for each clause
        i = 0
        while i < n:
            if label[i]:
                c = self.node_index_to_clause[i]
                i += 1
                while i < n and self.node_index_to_clause[i] == c:
                    label[i] = 0
                    i += 1
            else:
                i += 1

        return label.type(torch.uint8)

    def mis_to_cnf_solution(self, node_labeling, conflict_is_error=True):
        n = self.expected_num_vars
        bits = torch.zeros(n, dtype=torch.uint8)

        for bit_idx in range(n):
            lit = bit_idx + 1

            positive_nodes = self.lit_to_node_indices[lit]
            if node_labeling[positive_nodes].any():
                bits[bit_idx] = 1

                negative_nodes = self.lit_to_node_indices[-lit]
                if node_labeling[negative_nodes].any():
                    if conflict_is_error:
                        assert False, 'Conflicting assignment!'
                    else:
                        return None

        return bits

    def is_independent_set(self, label):
        n = label.size(0)
        assert n == self.g.num_nodes()

        # Only consider nodes labeled as "1"
        nodes = torch.arange(n)[label == 1]
        return dgl.node_subgraph(self.g, nodes).num_edges() == 0

    def is_sat(self, label):
        mis_size = torch.sum(label)
        return mis_size >= self.expected_num_clauses and \
            self.is_independent_set(label)
