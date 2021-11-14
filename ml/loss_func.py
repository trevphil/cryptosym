import dgl
import torch
from typing import Dict

from opts import PreimageOpts


class PreimageLoss(object):
    def __init__(self, opts: PreimageOpts):
        self.opts = opts
        self.bce = torch.nn.BCELoss()

    def __call__(
        self, batched_graph: dgl.DGLGraph, pred_dict: Dict, sat_labels: torch.Tensor
    ) -> Dict:
        bs = batched_graph.batch_size

        sat_pred = pred_dict["sat"]
        sat_loss = self.bce(sat_pred, sat_labels)

        num_incorrect = (sat_pred.round() - sat_labels).abs().sum()
        num_correct = bs - num_incorrect.detach().item()

        colorf = pred_dict["colors"]
        colori = torch.zeros_like(colorf)
        colori[torch.arange(colorf.size(0)), colorf.argmax(1)] = 1

        batched_graph.ndata["color_float"] = colorf
        batched_graph.ndata["color_int"] = colori
        graphs = dgl.unbatch(batched_graph)

        color_loss = 0.0
        total_color_conflicts = 0
        min_conflicts = 1e10
        num_solved_graphs = 0
        num_nodes = 0
        num_edges = 0

        for g in graphs:
            num_nodes += g.num_nodes()
            num_edges += g.num_edges()

            cf = g.ndata["color_float"]
            ci = g.ndata["color_int"]

            # Dot product of node features, for each edge
            edgef = dgl.ops.u_dot_v(g, cf, cf)
            color_loss += edgef.sum()

            edgei = dgl.ops.u_dot_v(g, ci, ci)
            color_conflicts = edgei.sum().detach().item()
            min_conflicts = min(min_conflicts, color_conflicts)
            if color_conflicts == 0:
                # A valid 3-coloring solution was found!
                num_solved_graphs += 1
            total_color_conflicts += color_conflicts

        color_loss /= num_edges
        loss = (
            self.opts.sat_loss_weight * sat_loss
            + self.opts.color_loss_weight * color_loss
        )

        mean_nodes = num_nodes / float(bs)
        mean_edges = num_edges / float(bs)
        frac_violated_edges = total_color_conflicts / num_edges

        return {
            "loss": loss,
            "sat_loss": sat_loss,
            "color_loss": color_loss,
            "num_correct_classifications": num_correct,
            "frac_violated_edges": frac_violated_edges,
            "min_conflicts": min_conflicts,
            "num_solved": num_solved_graphs,
            "mean_num_nodes": mean_nodes,
            "mean_num_edges": mean_edges,
        }
