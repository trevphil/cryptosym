import dgl
import torch
from typing import Dict
from dgl.nn import SetTransformerEncoder, SetTransformerDecoder, GlobalAttentionPooling


class HashSAT(torch.nn.Module):
    def __init__(self, t_max: int, hidden_size: int):
        super(HashSAT, self).__init__()
        self.num_colors = 3
        self.t_max = t_max
        self.hidden_size = hidden_size

        self.encoder = SetTransformerEncoder(
            d_model=self.hidden_size,
            n_heads=4,
            d_head=self.hidden_size,
            d_ff=20,
            n_layers=4,
        )
        self.decoder = SetTransformerDecoder(
            d_model=self.hidden_size,
            num_heads=4,
            d_head=self.hidden_size,
            d_ff=20,
            n_layers=4,
            k=1,
        )

        self.colorize = torch.nn.Linear(self.hidden_size, self.num_colors)
        self.sat_layer = torch.nn.Linear(self.hidden_size, 1)

        self.num_parameters = sum(p.numel() for p in self.parameters())

    @property
    def name(self) -> str:
        return "encoder_decoder"

    def forward(self, graph: dgl.DGLGraph) -> Dict[str, torch.Tensor]:
        # x = torch.rand((graph.num_nodes(), self.hidden_size), dtype=torch.float32)
        # x /= x.size(1) ** 0.5
        x = torch.ones((graph.num_nodes(), self.hidden_size), dtype=torch.float32)

        encoded = self.encoder(graph, x)

        node_coloring = self.colorize(encoded)
        node_coloring = torch.softmax(node_coloring, dim=1)

        decoded = self.decoder(graph, encoded)
        sat = self.sat_layer(decoded)
        sat = torch.sigmoid(sat).squeeze()

        return {"colors": node_coloring, "sat": sat}
