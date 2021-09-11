import torch
import dgl
from termcolor import cprint

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
            # Reduce the graph using a special MIS solver...
            _, new_node_label, _ = self.api.reduce_graph(g_unreduced)
            # Update solution stored in subgraph
            solution = g_unreduced.ndata["solution"]
            nodes_in_mis = torch.arange(g_unreduced.num_nodes())[new_node_label == 0]
            solution[nodes_in_mis] = 1
            nbrs = g_unreduced.in_edges(nodes_in_mis)[0]
            solution[nbrs] = 0

            # Update solution in ORIGINAL graph
            g.ndata["solution"][g_unreduced.ndata["orig_id"]] = solution.clone()
            # Remove all labeled nodes and get a subgraph of unlabeled
            g_reduced = dgl.node_subgraph(g, g.ndata["solution"] == -1)

            # Remove zero-degree nodes
            deg = g_reduced.in_degrees()
            zero_deg_nodes = torch.arange(g_reduced.num_nodes())[deg == 0]

            for node in zero_deg_nodes:
                orig_node = g_reduced.ndata["orig_id"][node]
                nbrs = g.in_edges(orig_node)[0]
                if g.ndata["solution"][nbrs].any():
                    g.ndata["solution"][orig_node] = 0
                else:
                    g.ndata["solution"][orig_node] = 1

            return dgl.node_subgraph(g, g.ndata["solution"] == -1)

        def _partial_solve(g_residual):
            g_residual = _reduce_graph(g_residual)
            if g_residual.num_nodes() == 0:
                return g_residual

            if no_model:
                nodes = torch.randperm(g_residual.num_nodes())
            else:
                # Do inference using model
                node_ranking = self.model(g_residual)
                # Take first solution
                node_ranking = node_ranking[:, 0]
                # Get current solution (-1 = unassigned)
                solution = g_residual.ndata["solution"]
                # Sort nodes from greatest to smallest ranking
                nodes = torch.argsort(node_ranking, descending=True)

            for node in nodes:
                # Label each ranked node as 1 and neighbors as 0
                if solution[node] != -1:
                    # If the current node is already labeled, stop
                    break
                solution[node] = 1
                nbrs = g_residual.in_edges(node)[0]
                solution[nbrs] = 0
            # Update solution in ORIGINAL graph
            g.ndata["solution"][g_residual.ndata["orig_id"]] = solution.clone()
            # Remove all labeled nodes and get a subgraph of unlabeled
            return dgl.node_subgraph(g, g.ndata["solution"] == -1)

        g_res = g
        while g_res.num_nodes() > 0:
            g_res = _partial_solve(g_res)

        assert mis.is_independent_set(g.ndata["solution"])
        mis_before = torch.sum(g.ndata["solution"] == 1)
        g.ndata["solution"] = self.api.local_search(g, g.ndata["solution"])
        mis_after = torch.sum(g.ndata["solution"] == 1)
        assert mis.is_independent_set(g.ndata["solution"])
        mis_max = mis.expected_num_clauses
        cprint(f"before: {mis_before}, after: {mis_after}, max: {mis_max}", "green")

        return g.ndata["solution"]
