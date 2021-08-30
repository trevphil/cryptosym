import torch
import dgl
from copy import deepcopy
from collections import defaultdict


class MaximumIndependentSet(object):
    def __init__(self, cnf):
        self.cnf_clauses = []
        self.node_clauses = []

        num_nodes = sum(len(clause) for clause in cnf.clauses)
        src, dst = [], []
        node_index = 0
        node_index_to_lit = torch.zeros(num_nodes, dtype=int)
        lit_to_node_indices = defaultdict(lambda: [])

        for clause_idx, clause in enumerate(cnf.clauses):
            assert clause is not None and len(clause) > 1

            num_lits = len(clause)
            clause = list(clause)
            nodes = list(range(node_index, node_index + num_lits))

            for i in range(num_lits):
                lit = clause[i]
                node = nodes[i]
                node_index_to_lit[node] = lit
                lit_to_node_indices[lit].append(node)

            for i in range(num_lits - 1):
                for j in range(i + 1, num_lits):
                    src.append(node_index + i)
                    dst.append(node_index + j)

            self.cnf_clauses.append(clause)
            self.node_clauses.append(nodes)
            node_index += num_lits

        for lit, set_a in lit_to_node_indices.items():
            if (-lit) not in lit_to_node_indices:
                continue
            set_b = lit_to_node_indices[-lit]
            for a in set_a:
                for b in set_b:
                    src.append(a)
                    dst.append(b)

        assert node_index == num_nodes, f'{node_index} != {num_nodes}'
        g = dgl.graph((src, dst), num_nodes=num_nodes, idtype=torch.int64)
        g = dgl.to_simple(g)  # Remove parallel edges
        g = dgl.to_bidirected(g)  # Make bidirectional <-->

        self.g = g
        self.cnf = cnf
        self.node_index_to_lit = node_index_to_lit
        self.lit_to_node_indices = lit_to_node_indices

    @property
    def num_nodes(self):
        return self.g.num_nodes()

    @property
    def num_edges(self):
        return self.g.num_edges()

    def cnf_to_mis_solution(self, bits):
        """
        For each clause in the CNF:
            Pick a literal in the clause with truth value = 1 and
                corresponding node in the graph unassigned
            Set label = 1 to the corresponding node X in the graph for this literal
            Set label = 0 to all Neighbors(X)
            Remove X and Neighbors(X) from the set of unlabeled nodes
        """
        node_labeling = torch.ones(self.num_nodes, dtype=torch.int32) * -1
        n_clauses = len(self.cnf_clauses)

        """
        values = torch.zeros(self.num_nodes, dtype=bits.dtype)
        lit_mask = (self.node_index_to_lit > 0)
        lits = self.node_index_to_lit[lit_mask]  # positive literals
        values[lit_mask] = bits[lits - 1]
        lit_mask = ~lit_mask
        lits = self.node_index_to_lit[lit_mask]  # negative literals
        values[lit_mask] = 1 - bits[-lits - 1]
        """

        for i in range(n_clauses):
            clause = self.cnf_clauses[i]
            nodes = self.node_clauses[i]
            selected_node = None

            for lit, node in zip(clause, nodes):
                if node_labeling[node] != -1:
                    continue  # Only consider unassigned nodes

                bit_val = bits[abs(lit) - 1]
                if lit < 0:
                    bit_val = 1 - bit_val

                if bit_val:
                    selected_node = node
                    break

            if selected_node is None:
                print(f'{clause}, {nodes}, {node_labeling[nodes]}')
                return None  # UNSAT

            node_labeling[selected_node] = 1
            nbrs = self.g.in_edges(selected_node, form='uv')[0]
            assert not (node_labeling[nbrs] == 1).any(), 'MIS has connected vertices!'
            node_labeling[nbrs] = 0

        assert not (node_labeling == -1).any(), 'Not all nodes were labeled!'
        mis_size = torch.sum(node_labeling).item()
        assert mis_size == n_clauses, f'|MIS| = {mis_size}, but {n_clauses} clauses'

        return node_labeling.type(torch.uint8)

    def mis_to_cnf_solution(self, node_labeling, conflict_is_error=True):
        n = self.cnf.num_vars
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
