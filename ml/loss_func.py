import dgl
import torch
from collections import defaultdict

from logic.gate import GateType, gate_type_to_str


class PreimageLoss(object):
    def __init__(self):
        pass

    def bce(self, batched_graph, labels, num_candidate_sol):
        graphs = dgl.unbatch(batched_graph)
        cum_loss = 0.0
        votes = torch.zeros(num_candidate_sol, dtype=int)

        # BCE = - [ y * log(x) + (1 - y) * log(1 - x) ]
        for i, g in enumerate(graphs):
            # N x P x 1
            pred = g.ndata["pred"][:, :, None]
            assert num_candidate_sol == pred.size(1)

            # N x 1 x Q
            label = labels[i][:, None, :].to(pred)
            assert label.size(0) == pred.size(0)

            # N x P x Q
            tiled_label = torch.tile(label, (1, num_candidate_sol, 1))

            # N x P x Q
            term1 = tiled_label * torch.log(pred)
            term2 = (1 - tiled_label) * torch.log(1 - pred)
            bce_loss = torch.maximum(-(term1 + term2), torch.tensor(-100.0))

            # P x Q
            reduced = bce_loss.mean(axis=0)

            # Take best across all predicted x GT solution combinations
            cum_loss += reduced.mean(axis=0).min()
            
            # Increase vote for solution index with minimum loss
            votes[reduced.min(axis=1)[0].argmin().detach()] += 1

        return cum_loss / len(graphs), votes

    def mis_size_ratio(self, mis_samples, batched_graph):
        num_samples = len(mis_samples)
        graphs = dgl.unbatch(batched_graph)
        assert num_samples == len(graphs)
        cum_score = 0.0

        for i, mis in enumerate(mis_samples):
            max_mis_size = mis.expected_num_clauses

            g = graphs[i]
            num_nodes = g.num_nodes()
            pred = g.ndata["pred"]
            num_candidate_solutions = pred.size(1)
            best_mis_size = 0

            for sol_index in range(num_candidate_solutions):
                label = torch.ones(num_nodes, dtype=int) * -1
                sorted_nodes = torch.argsort(pred[:, sol_index], descending=True)

                for node in sorted_nodes:
                    if label[node] != -1:
                        break  # Node is already labeled
                    label[node] = 1
                    nbrs = g.in_edges(node)[0]
                    label[nbrs] = 0

                # Find the solution corresponding to largest independent set
                mis_size = torch.sum(label[label != -1]).detach().item()
                best_mis_size = max(best_mis_size, int(mis_size))

            # If this ratio == 1.0, the problem was solved
            cum_score += best_mis_size / float(max_mis_size)

        return cum_score / num_samples
