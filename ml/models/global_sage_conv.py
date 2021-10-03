import dgl
import torch
from typing import Dict
from torch.nn import functional as F
from dgl.nn import GraphConv, GlobalAttentionPooling, SAGEConv


class HashSAT(torch.nn.Module):
    def __init__(self, t_max: int, hidden_size: int):
        super(HashSAT, self).__init__()
        self.num_colors = 3
        self.t_max = t_max
        self.hidden_size = hidden_size

        self.sage_conv = SAGEConv(self.hidden_size * 2, self.hidden_size, "lstm")
        self.graph_conv = GraphConv(self.hidden_size * 2, self.num_colors)

        self.global_state_nn = torch.nn.Linear(self.hidden_size, 1)
        self.global_state = GlobalAttentionPooling(self.global_state_nn)

        self.final_pooling_nn = torch.nn.Linear(self.num_colors, 1)
        self.final_pooling = GlobalAttentionPooling(self.final_pooling_nn)
        self.sat_classifier = torch.nn.Linear(self.num_colors, 1)

        self.num_parameters = sum(p.numel() for p in self.parameters())

    @property
    def name(self) -> str:
        return "SageConv_GlobalState"

    def forward(self, graph: dgl.DGLGraph) -> Dict[str, torch.Tensor]:
        batch_num_nodes = graph.batch_num_nodes()
        x = torch.rand((graph.num_nodes(), self.hidden_size * 2), dtype=torch.float32)
        x /= x.size(1) ** 0.5

        for _ in range(self.t_max):
            x = self.sage_conv(graph, x)
            x = F.leaky_relu(x, 1e-2)
            gstate = self.global_state(graph, x)
            gstate = torch.repeat_interleave(gstate, batch_num_nodes, dim=0)
            x = torch.cat((x, gstate), dim=1)

        x = self.graph_conv(graph, x)  # [num_nodes x 3]
        x = F.softmax(x, dim=1)  # [num_nodes x 3]
        node_coloring = x  # [num_nodes x 3]
        x = self.final_pooling(graph, x)  # [batch_size x 3]
        x = self.sat_classifier(x)  # [batch_size x 1]
        sat = F.sigmoid(x).squeeze()

        return {"colors": node_coloring, "sat": sat}
