import uuid
import torch
import dgl
from termcolor import cprint
from randomdict import RandomDict
from time import time

from solver.kamis import KaMIS


class TreeMISSolver(object):
    def __init__(self, model, solution_weights):
        self.model = model
        self.solution_weights = solution_weights
        self.api = KaMIS()

    def random_uuid(self):
        return str(uuid.uuid4())

    def reduce_graph(self, g, g_unreduced, solution):
        orig_id = g_unreduced.ndata["orig_id"]
        # Reduce the graph using a special MIS solver...
        _, new_node_label, _ = self.api.reduce_graph(g_unreduced)
        # Update solution
        nodes_in_mis = orig_id[new_node_label == 0]
        mis_neighbors = g.in_edges(nodes_in_mis)[0]
        solve_later = orig_id[new_node_label == 2]
        solution[solve_later] = -1 # 2
        solution[nodes_in_mis] = 1
        solution[mis_neighbors] = 0

        # Remove all labeled nodes and get a subgraph of unlabeled
        g_reduced = dgl.node_subgraph(g, solution == -1)

        # Remove zero-degree nodes in subgraph (mark them as 1)
        deg = g_reduced.in_degrees()
        zero_deg_nodes = g_reduced.ndata["orig_id"][deg == 0]
        zero_deg_neighbors = g.in_edges(zero_deg_nodes)[0]
        solution[zero_deg_nodes] = 1
        solution[zero_deg_neighbors] = 0

        g_reduced = dgl.node_subgraph(g, solution == -1)
        return g_reduced, solution

    def solve(self, mis, g, no_model=False):
        n = g.num_nodes()
        g.ndata["solution"] = torch.ones(n, dtype=int) * -1
        g.ndata["orig_id"] = torch.arange(n, dtype=int)
        mis_max = mis.expected_num_clauses

        num_terminal_leaves = 0
        largest_mis, best_solution = None, None

        queue = RandomDict()
        queue[self.random_uuid()] = (g, g.ndata["solution"].clone())

        start = time()
        max_runtime_sec = 10.0
        while time() - start < max_runtime_sec and len(queue) > 0:
            key = queue.random_key()
            g_res, incomplete_sol = queue[key]  # Pop random
            del queue[key]

            g_res, incomplete_sol = self.reduce_graph(g, g_res, incomplete_sol)

            # Do inference using the NN model or random numbers
            if no_model:
                node_rank = torch.randn(g_res.num_nodes(), 32)
            else:
                node_rank = self.model(g_res)

            # Sort nodes from greatest to smallest ranking
            node_idx = torch.argsort(node_rank, descending=False, axis=0)

            num_solutions = node_rank.size(1)
            for sol_idx in range(num_solutions):
                if self.solution_weights[sol_idx] == 0:
                    continue  # Model doesn't predict a good solution here

                solution = incomplete_sol.clone()
                nodes = g_res.ndata["orig_id"][node_idx[:, sol_idx]]

                for node in nodes:
                    # Sequentially label each node as 1 and neighbors as 0
                    if solution[node] != -1:
                        # If the current node is already labeled, stop
                        break
                    solution[node] = 1
                    nbrs = g.in_edges(node)[0]
                    solution[nbrs] = 0

                if not (solution == -1).any():
                    # We've reached a terminal node...
                    # Solve for all nodes labeled 2 ( = "solve later")
                    g_final = dgl.node_subgraph(g, solution == 2)
                    while g_final.num_nodes() > 0:
                        _, node_label, _ = self.api.reduce_graph(g_final)
                        nodes_in_mis = g_final.ndata["orig_id"][node_label == 0]
                        mis_neighbors = g.in_edges(nodes_in_mis)[0]
                        solution[nodes_in_mis] = 1
                        solution[mis_neighbors] = 0
                        g_final = dgl.node_subgraph(g, solution == 2)

                    solution = self.api.local_search(g, solution)
                    mis_size = torch.sum(solution == 1).detach().item()
                    num_terminal_leaves += 1

                    if mis_size >= mis_max:
                        cprint(f"Found optimal solution! {mis_max}", "green")
                        return solution  # Found a SAT assignment!
                    elif largest_mis is None or mis_size > largest_mis:
                        largest_mis = mis_size
                        best_solution = solution
                        cprint(f"Found new largest MIS, {largest_mis} / {mis_max}", "green")
                    else:
                        cprint(f"Got terminal node, {mis_size} / {mis_max}")
                else:
                    # Not all nodes got labeled, so add subgraph to queue
                    g_unlabeled = dgl.node_subgraph(g, solution == -1)
                    queue[self.random_uuid()] = (g_unlabeled, solution)

        cprint(f"Timeout ({max_runtime_sec} sec, {num_terminal_leaves} leaves), best is {largest_mis} / {mis_max}", "yellow")
        return best_solution
