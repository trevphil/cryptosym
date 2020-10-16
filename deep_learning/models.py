# -*- coding: utf-8 -*-

import torch
import numpy as np
from torch import nn
from time import time


class ReverseHashModel(nn.Module):
    def __init__(self, config, factors, feat_size):
        super(ReverseHashModel, self).__init__()
        start = time()
        self.factors = factors
        self.sorted_rvs = list(sorted(factors.keys()))
        self.num_input_bits = int(config['num_input_bits'])
        self.bits_per_sample = int(config['num_bits_per_sample'])
        self.n = len(self.sorted_rvs)
        self.feat_size = feat_size

        A_fwd = torch.eye(self.n, requires_grad=False)
        A_bwd = torch.eye(self.n, requires_grad=False)
        for rv, factor in self.factors.items():
            for inp in factor.input_rvs:
                A_fwd[inp, rv] = 1 # (0, 2), (1, 2), (1, 3)
                A_bwd[rv, inp] = 1
        self.A_fwd = self.normalized_adjacency_mat(A_fwd)
        self.A_bwd = self.normalized_adjacency_mat(A_bwd)
        self.W0 = self.init_w(self.feat_size, 10)
        self.W1 = self.init_w(10, 5)
        self.W2 = self.init_w(5, 1)
        self.relu = nn.ReLU()

        # relu(A * X * W) --> [nxn] * [nx1] * [1x4] = [nx4]

        print('Initialized ReverseHashModel in %.2f s' % (time() - start))
        num_p = sum(p.numel() for p in self.parameters())
        print('Number of parameters: %d' % num_p)

    def normalized_adjacency_mat(self, A, spectral=True):
        if spectral:
            D = 1.0 / torch.sqrt(torch.sum(A, 0).squeeze())
            D = torch.diag(D)
            return D @ A @ D
        else:
            D = 1.0 / torch.sum(A, 0).squeeze()
            D = torch.diag(D)
            return D @ A

    def init_w(self, m, n):
        w = torch.empty((m, n))
        torch.nn.init.kaiming_normal_(w)
        return nn.Parameter(w, requires_grad=True)

    def graph_conv(self, x, w, activation=True):
        out = self.A @ x @ w
        return self.relu(out) if activation else out

    def forward(self, x):
        x = x.squeeze(dim=0)
        out0 = self.graph_conv(x, self.W0)
        out1 = self.graph_conv(out0, self.W1)
        out2 = self.graph_conv(out1, self.W2, activation=False)
        return out2.squeeze()
