import torch
from dgl.nn import GraphConv, GlobalAttentionPooling


class HashSAT(torch.nn.Module):
    def __init__(self, num_layers, hidden_size):
        super(HashSAT, self).__init__()
        self.num_colors = 3
        self.num_layers = num_layers
        self.hidden_size = hidden_size

        sizes = [hidden_size for _ in range(num_layers)] + [self.num_colors]
        self.layers = torch.nn.ModuleList()
        for layer_idx in range(self.num_layers):
            in_sz = sizes[layer_idx]
            out_sz = sizes[layer_idx + 1]
            activ = None if layer_idx == (num_layers - 1) else torch.relu
            conv = GraphConv(in_sz, out_sz, activation=activ, allow_zero_in_degree=True)
            # conv = GATConv(in_sz, out_sz, 3, activation=activ)
            self.layers.append(conv)

        self.pooling_nn = torch.nn.Linear(self.num_colors, 1)
        self.pooling = GlobalAttentionPooling(self.pooling_nn)
        self.output_layer = torch.nn.Linear(self.num_colors, 1)
        self.sigmoid = torch.nn.Sigmoid()
        self.softmax = torch.nn.Softmax(dim=1)

        self.num_parameters = sum(p.numel() for p in self.parameters())
        # print(f'{self.name} has {num_parameters} parameters.')

    @property
    def name(self):
        return "HashSAT"

    def forward(self, graph):
        x = torch.rand((graph.num_nodes(), self.hidden_size), dtype=torch.float32)
        x /= self.hidden_size ** 0.5
        for conv in self.layers:
            x = conv(graph, x)
        x = self.softmax(x)  # [num_nodes x 3]
        node_coloring = x  # [num_nodes x 3]
        x = self.pooling(graph, x)  # [batch_size x 3]
        x = self.output_layer(x)  # [batch_size x 1]
        sat = self.sigmoid(x).squeeze()
        return {"colors": node_coloring, "sat": sat}
