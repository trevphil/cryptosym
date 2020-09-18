# -*- coding: utf-8 -*-

import torch
import numpy as np
from torch import nn
from time import time
from heapq import heappop, heappush, heapify
from collections import defaultdict

class PriorNode(nn.Module):
    def __init__(self, ident, num_parents):
        super(PriorNode, self).__init__()
        self.ident = ident
        self.num_parents = max(1, num_parents)
        self.fc = nn.Linear(self.num_parents, 1)
        self.sig = nn.Sigmoid()
        self.relu = nn.ReLU()

    def forward(self, x):
        batch_size = x.size(0)
        x = torch.reshape(x, (batch_size, self.num_parents))
        x = self.fc(x)
        # x = self.sig(x)
        x = self.relu(x)
        x = torch.reshape(x, (batch_size, 1))
        return x

class InvNode(nn.Module):
    def __init__(self, ident, num_parents):
        super(InvNode, self).__init__()
        self.ident = ident
        self.num_parents = max(1, num_parents)
        self.fc = nn.Linear(self.num_parents, 1)
        self.sig = nn.Sigmoid()
        self.relu = nn.ReLU()

    def forward(self, x):
        batch_size = x.size(0)
        x = torch.reshape(x, (batch_size, self.num_parents))
        x = self.fc(1.0 - x)
        # x = self.sig(x)
        x = self.relu(x)
        x = torch.reshape(x, (batch_size, 1))
        return x

class AndNode(nn.Module):
    def __init__(self, ident, num_parents):
        super(AndNode, self).__init__()
        self.ident = ident
        self.num_parents = max(1, num_parents)
        self.fc = nn.Linear(self.num_parents, 1)
        self.sig = nn.Sigmoid()
        self.relu = nn.ReLU()

    def forward(self, x):
        batch_size = x.size(0)
        x = torch.reshape(x, (batch_size, self.num_parents))
        x = self.fc(x)
        # x = self.sig(x)
        x = self.relu(x)
        x = torch.reshape(x, (batch_size, 1))
        return x

"""
      ------ hash inputs ------
    1        2         3        4
     |      |          ||      |
      |    |          |  |    |
       |  |          |    |  |
         5          |       6
         |         |        |
         |        |         |
         7       |          8
          |     |         | |
           |   |        |   |
             9        |     |
              |     |       |
                | |         |
                10          11
                --- outputs ---

  1 -> []
  2 -> []
  3 -> []
  4 -> []
  5 -> [1, 2]
  6 -> [3, 4]
  7 -> [5]
  8 -> [6]
  9 -> [3, 7]
  10 -> [8, 9]
  11 -> [8]

            heap: init with hash RVs (order: bigger->smaller)
            [11, 10]      node_idx: [inputs]
  11        [10, 8]       8: [a]             <-- each element in 1 list is a tensor
  10        [9, 8]        9: [x], 8: [a, b]      need to average them, take max, something...
  9         [8, 7, 3]     8: [a, b], 7: [w], 3: [k]
  8         [7, 6, 3]     7: [w], 6: [g], 3: [k]
  7         [6, 5, 3]     6: [g], 5: [u], 3: [k]
  6         [5, 4, 3]     5: [u], 4: [i], 3: [k, l]
  5         [4, 3, 2, 1]  4: [i], 3: [k, l], 2: [r], 1: [p]
  4         [3, 2, 1]     3: [k, l], 2: [r], 1: [p]            output[4] = node4(i)
  3         [2, 1]        2: [r], 1: [p]                       output[3] = node3(k, l)
  2         [1]           1: [p]                               output[2] = node2(r)
  1         []            -                                    output[1] = node1(p)

  Fix the size of the output tensor for INV node. Output for AND is twice that size.
  The input size for each node can be determined based on:
    num_inputs(RV) = # factors where RV is one of the inputs

"""

class ReverseHashModel(nn.Module):
    def __init__(self, config, factors, observed_rvs,
                 obs_rv2idx, parents_per_rv):
        super(ReverseHashModel, self).__init__()
        start = time()
        self.factors = factors
        self.sorted_rvs = list(sorted(factors.keys()))
        self.observed_rvs = np.array(observed_rvs, dtype=int)
        self.obs_rv_set = set(observed_rvs)
        self.obs_rv2idx = obs_rv2idx
        self.num_input_bits = int(config['num_input_bits'])
        self.bits_per_sample = int(config['num_bits_per_sample'])
        self.num_parents = parents_per_rv

        # self.add_connections()

        self.nodes = dict()
        for rv, factor in self.factors.items():
            node = self.make_node(factor)
            self.nodes[rv] = node
            self.add_module(str(rv), node)

        print('Initialized ReverseHashModel in %.2f s' % (time() - start))

    def add_connections(self):
        available_rvs = set(self.factors.keys())
        rvs = sorted(available_rvs)
        for rv in rvs:
            if rv < self.num_input_bits:
                # Input bit RVs do not have inputs
                continue
            existing = set(self.factors[rv].input_rvs)
            new_inputs = available_rvs.intersection(set(range(rv))).difference(existing)

            for new_input in new_inputs:
                self.factors[rv].input_rvs.append(new_input)
                self.num_parents[new_input] += 1

    def forward(self, observed_bits):
        start = time()
        batch_size = observed_bits.shape[0]
        node_inputs = defaultdict(lambda: [])
        queue = (-self.observed_rvs).tolist()
        queue_set = set(queue)
        heapify(queue)

        dft_input = torch.zeros((batch_size, 1), requires_grad=False)
        bit_predictions = defaultdict(lambda: dft_input)

        while len(queue) > 0:
            rv = heappop(queue)
            queue_set.remove(rv)
            rv = -rv
            if rv in self.obs_rv_set:
                i = self.obs_rv2idx[rv]
                node_input = observed_bits[:, i].squeeze().item()
                shape = (batch_size, 1, max(1, self.num_parents[rv]))
                node_input = torch.ones(shape) * node_input
            else:
                node_input = self.concat_inputs(node_inputs[rv])
            node_output = self.nodes[rv](node_input)

            if rv < self.num_input_bits:
                bit_predictions[rv] = node_output

            if node_inputs[rv]:
                del node_inputs[rv]

            factor = self.factors[rv]
            for child in factor.input_rvs:
                node_inputs[child].append(node_output)
                child = -child
                if child not in queue_set:
                    heappush(queue, child)
                    queue_set.add(child)

        pred = torch.zeros((batch_size, 0), requires_grad=True)
        for rv in range(self.num_input_bits):
            pred = torch.cat((pred, bit_predictions[rv]), axis=1)

        # print('Forward pass completed in %.2f s' % (time() - start))
        return pred.squeeze()

    def concat_inputs(self, list_of_inputs):
        batch_size = list_of_inputs[0].size(0)
        result = torch.zeros((batch_size, 0), requires_grad=True)
        for inp in list_of_inputs:
            result = torch.cat((result, inp), axis=1)
        return result

    def make_node(self, factor):
        ident = factor.output_rv
        ftype = factor.factor_type
        num_inputs = self.num_parents[factor.output_rv]
        if ftype == 'AND':
            return AndNode(ident, num_inputs)
        elif ftype == 'INV':
            return InvNode(ident, num_inputs)
        elif ftype == 'PRIOR':
            return PriorNode(ident, num_inputs)
