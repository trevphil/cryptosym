import torch
import dgl
from dgl.nn import GraphConv, HeteroGraphConv, GatedGraphConv
from collections import defaultdict

from logic.gate import GateType, gate_type_to_str


class HashSAT_GGNN(torch.nn.Module):
    def __init__(self, feat_size, out_feats,
                 n_steps, n_etypes):
        super(HashSAT, self).__init__()

        print(f'feat_size={feat_size}; out_feats={out_feats}; n_steps={n_steps}; n_etypes={n_etypes}')

        self.feat_size = feat_size
        self.out_feats = out_feats

        self.ggnn = GatedGraphConv(in_feats=out_feats,
                                   out_feats=out_feats,
                                   n_steps=n_steps,
                                   n_etypes=n_etypes)

        self.output_layer = torch.nn.Linear(feat_size + out_feats, 1)

        num_parameters = sum(p.numel() for p in self.parameters())
        print(f'{self.name} has {num_parameters} parameters.')

    @property
    def name(self):
        return 'HashSAT'

    def forward(self, graph, feat):
        etypes = graph.edata['type']

        assert feat.size()[-1] == self.feat_size

        node_num = graph.number_of_nodes()

        if self.out_feats > self.feat_size:
            zero_pad = torch.zeros([node_num, self.out_feats - self.feat_size],
                                   dtype=torch.float, device=feat.device)
            h1 = torch.cat([feat, zero_pad], -1)
        else:
            h1 = feat

        out = self.ggnn(graph, h1, etypes)
        out = self.output_layer(torch.cat([out, feat], -1)).squeeze(-1)
        out = torch.sigmoid(out)
        # print(out)
        return out
