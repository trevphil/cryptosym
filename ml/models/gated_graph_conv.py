import dgl
import torch
from typing import Dict
from dgl.nn import GraphConv, GlobalAttentionPooling, GatedGraphConv


class HashSAT(torch.nn.Module):
    def __init__(self, t_max: int, hidden_size: int):
        super(HashSAT, self).__init__()
        self.num_colors = 3
        self.t_max = t_max
        self.hidden_size = hidden_size

        self.ggc = GatedGraphConv(self.hidden_size, self.hidden_size, self.t_max, 1)
        self.gc = GraphConv(self.hidden_size, self.num_colors)
        self.layers = [self.ggc, self.gc]

        self.pooling_nn = torch.nn.Linear(self.num_colors, 1)
        self.pooling = GlobalAttentionPooling(self.pooling_nn)
        self.output_layer = torch.nn.Linear(self.num_colors, 1)

        self.num_parameters = sum(p.numel() for p in self.parameters())

    @property
    def name(self) -> str:
        return "HashSAT"

    def forward(self, graph: dgl.DGLGraph) -> Dict[str, torch.Tensor]:
        x = torch.rand((graph.num_nodes(), self.hidden_size), dtype=torch.float32)
        x /= x.size(1) ** 0.5

        for i, conv in enumerate(self.layers):
            x = conv(graph, x)

        x = torch.softmax(x, dim=1)  # [num_nodes x 3]
        node_coloring = x  # [num_nodes x 3]
        x = self.pooling(graph, x)  # [batch_size x 3]
        x = self.output_layer(x)  # [batch_size x 1]
        sat = torch.sigmoid(x).squeeze()

        return {"colors": node_coloring, "sat": sat}
