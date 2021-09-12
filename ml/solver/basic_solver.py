import torch
import dgl
from termcolor import cprint
from random import randint

from solver.kamis import KaMIS


class BasicMISSolver(object):
    def __init__(self, model):
        self.model = model
        self.api = KaMIS()

    def solve(self, mis, g, no_model=False):
        n = g.num_nodes()
        g.ndata["solution"] = torch.ones(n, dtype=int) * -1
        g.ndata["orig_id"] = torch.arange(n, dtype=int)

        def _reduce_graph(g_unreduced):
            orig_id = g_unreduced.ndata["orig_id"]
            # Reduce the graph using a special MIS solver...
            _, new_node_label, _ = self.api.reduce_graph(g_unreduced)
            # Update solution
            nodes_in_mis = orig_id[new_node_label == 0]
            mis_neighbors = g.in_edges(nodes_in_mis)[0]
            solve_later = orig_id[new_node_label == 2]
            g.ndata["solution"][solve_later] = 2
            g.ndata["solution"][nodes_in_mis] = 1
            g.ndata["solution"][mis_neighbors] = 0

            # Remove all labeled nodes and get a subgraph of unlabeled
            g_reduced = dgl.node_subgraph(g, g.ndata["solution"] == -1)

            # Remove zero-degree nodes in subgraph (mark them as 1)
            deg = g_reduced.in_degrees()
            zero_deg_nodes = g_reduced.ndata["orig_id"][deg == 0]
            zero_deg_neighbors = g.in_edges(zero_deg_nodes)[0]
            g.ndata["solution"][zero_deg_nodes] = 1
            g.ndata["solution"][zero_deg_neighbors] = 0

            return dgl.node_subgraph(g, g.ndata["solution"] == -1)

        def _partial_solve(g_residual):
            g_residual = _reduce_graph(g_residual)
            if g_residual.num_nodes() == 0:
                return g_residual

            if no_model:
                node_idx = torch.randperm(g_residual.num_nodes())
            else:
                # Do inference using model
                node_ranking = self.model(g_residual)
                # Take random solution
                num_solutions = node_ranking.size(1)
                node_ranking = node_ranking[:, randint(0, num_solutions - 1)]
                # Sort nodes from greatest to smallest ranking
                node_idx = torch.argsort(node_ranking, descending=True)

            nodes = g_residual.ndata["orig_id"][node_idx]

            for node in nodes:
                # Sequentially label each node as 1 and neighbors as 0
                if g.ndata["solution"][node] != -1:
                    # If the current node is already labeled, stop
                    break
                g.ndata["solution"][node] = 1
                nbrs = g.in_edges(node)[0]
                g.ndata["solution"][nbrs] = 0

            # Remove all labeled nodes and get a subgraph of unlabeled
            return dgl.node_subgraph(g, g.ndata["solution"] == -1)

        g_res = g
        while g_res.num_nodes() > 0:
            g_res = _partial_solve(g_res)

        # Finally, solve for all nodes labeled 2 ("solve later")
        g_res = dgl.node_subgraph(g, g.ndata["solution"] == 2)
        while g_res.num_nodes() > 0:
            _, new_node_label, _ = self.api.reduce_graph(g_res)
            nodes_in_mis = g_res.ndata["orig_id"][new_node_label == 0]
            mis_neighbors = g.in_edges(nodes_in_mis)[0]
            g.ndata["solution"][nodes_in_mis] = 1
            g.ndata["solution"][mis_neighbors] = 0
            g_res = dgl.node_subgraph(g, g.ndata["solution"] == 2)
        sol_pre_local_search = g.ndata["solution"]
        assert ((sol_pre_local_search == 0) | (sol_pre_local_search == 1)).all()

        # Apply local search to improve the solution
        assert mis.is_independent_set(sol_pre_local_search)
        mis_before = torch.sum(sol_pre_local_search == 1)
        g.ndata["solution"] = self.api.local_search(g, sol_pre_local_search)
        mis_after = torch.sum(g.ndata["solution"] == 1)
        assert mis.is_independent_set(g.ndata["solution"])
        mis_max = mis.expected_num_clauses
        cprint(f"before: {mis_before}, after: {mis_after}, max: {mis_max}", "green")

        return g.ndata["solution"]
