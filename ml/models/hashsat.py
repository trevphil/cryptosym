import torch
import dgl
from dgl.nn import GraphConv, GATConv

from logic.gate import GateType, gate_type_to_str


class HashSAT(torch.nn.Module):
    def __init__(self, num_layers, hidden_size, num_solutions):
        super(HashSAT, self).__init__()
        self.num_layers = num_layers
        self.hidden_size = hidden_size
        self.num_solutions = num_solutions

        sizes = [hidden_size for _ in range(num_layers)] + [num_solutions]
        self.layers = torch.nn.ModuleList()
        for layer_idx in range(num_layers):
            in_sz = sizes[layer_idx]
            out_sz = sizes[layer_idx + 1]
            activ = torch.sigmoid if layer_idx == (num_layers - 1) else torch.relu
            conv = GraphConv(in_sz, out_sz, activation=activ)
            # conv = GATConv(in_sz, out_sz, 16, activation=activ)
            self.layers.append(conv)

        self.num_parameters = sum(p.numel() for p in self.parameters())
        # print(f'{self.name} has {num_parameters} parameters.')

    @property
    def name(self):
        return "HashSAT"

    def forward(self, graph):
        H = torch.ones((graph.num_nodes(), self.hidden_size), dtype=torch.float32)
        for conv in self.layers:
            H = conv(graph, H)
        return H
