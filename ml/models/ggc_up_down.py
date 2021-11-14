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
        D = 1024

        self.ggc = GatedGraphConv(self.hidden_size, self.hidden_size, self.t_max, 1)
        self.gc_upsample = GraphConv(self.hidden_size, D)
        self.gc_downsample = GraphConv(D, self.num_colors)

        self.leaky_relu = torch.nn.LeakyReLU()
        self.pooling_nn = torch.nn.Linear(D, 1)
        self.pooling = GlobalAttentionPooling(self.pooling_nn)
        self.output_layer = torch.nn.Linear(D, 1)

        self.num_parameters = sum(p.numel() for p in self.parameters())

    @property
    def name(self) -> str:
        return "ggc_upsample_downsample"

    def forward(self, graph: dgl.DGLGraph) -> Dict[str, torch.Tensor]:
        x = torch.rand((graph.num_nodes(), self.hidden_size), dtype=torch.float32)
        x /= x.size(1) ** 0.5

        x = self.ggc(graph, x)
        x_up = self.gc_upsample(graph, x)
        x_up = self.leaky_relu(x_up)
        x_down = self.gc_downsample(graph, x_up)

        node_coloring = torch.softmax(x_down, dim=1)

        pooled = self.pooling(graph, x_up)
        sat = self.output_layer(pooled)
        sat = torch.sigmoid(sat).squeeze()

        return {"colors": node_coloring, "sat": sat}
