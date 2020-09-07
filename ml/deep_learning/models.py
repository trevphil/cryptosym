# -*- coding: utf-8 -*-

import torch
import numpy as np
from torch import nn
from time import time
from heapq import heappop, heappush, heapify
from collections import defaultdict

S = 4

class PriorNode(nn.Module):
  def __init__(self, ident, num_parents):
    super(PriorNode, self).__init__()
    self.ident = ident
    self.num_parents = max(1, num_parents)
    self.fc = nn.Linear(self.num_parents * S, 1)
    self.sig = nn.Sigmoid()

  def forward(self, x):
    batch_size = x.size()[0]
    x = torch.reshape(x, (batch_size, self.num_parents * S))
    x = self.fc(x)
    x = self.sig(x)
    x = torch.squeeze(x)
    return x

class SameNode(nn.Module):
  def __init__(self, ident, num_parents):
    super(SameNode, self).__init__()
    self.ident = ident
    self.num_parents = max(1, num_parents)
    self.fc = nn.Linear(self.num_parents * S, S)

  def forward(self, x):
    batch_size = x.size()[0]
    x = torch.reshape(x, (batch_size, self.num_parents * S))
    x = self.fc(x)
    x = torch.reshape(x, (batch_size, S, 1))
    return x

class InvNode(nn.Module):
  def __init__(self, ident, num_parents):
    super(InvNode, self).__init__()
    self.ident = ident
    self.num_parents = max(1, num_parents)
    self.fc = nn.Linear(self.num_parents * S, S)

  def forward(self, x):
    batch_size = x.size()[0]
    x = torch.reshape(x, (batch_size, self.num_parents * S))
    x = self.fc(1.0 - x)
    x = torch.reshape(x, (batch_size, S, 1))
    return x

class AndNode(nn.Module):
  def __init__(self, ident, num_parents):
    super(AndNode, self).__init__()
    self.ident = ident
    self.num_parents = max(1, num_parents)
    self.fc = nn.Linear(self.num_parents * S, 2 * S)

  def forward(self, x):
    batch_size = x.size()[0]
    x = torch.reshape(x, (batch_size, self.num_parents * S))
    x = self.fc(x)
    x = torch.reshape(x, (batch_size, S, 2))
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

  Fix the size of the output tensor for SAME, INV nodes. Output for AND is twice that size.
  The input size for each node can be determined based on:
    num_inputs(RV) = # factors where RV is one of the inputs

"""

class ReverseHash(nn.Module):
  def __init__(self, factors, obs_rv_set, num_input_bits):
    super(ReverseHash, self).__init__()
    start = time()
    self.factors = factors
    self.obs_rv_indices = np.array(sorted(obs_rv_set), dtype=int)
    self.obs_rv2idx = {rv: i for i, rv in enumerate(self.obs_rv_indices)}
    self.obs_rv_set = obs_rv_set
    self.num_input_bits = num_input_bits

    self.num_parents = defaultdict(lambda: 0)
    for _, factor in factors.items():
      for input_rv in factor.input_rvs:
        self.num_parents[input_rv] += 1

    self.nodes = dict()
    for rv, factor in factors.items():
      node = self.make_node(factor)
      self.nodes[rv] = node
      self.add_module(str(rv), node)

    print('Initialized ReverseHash in %.2f s' % (time() - start))

  def forward(self, x):
    start = time()
    batch_size = x.shape[0]
    msg_prediction = torch.zeros((batch_size, self.num_input_bits))
    queue = (-self.obs_rv_indices).tolist()
    queue_set = set(queue)
    heapify(queue)

    node_inputs = defaultdict(lambda: [])
    while len(queue) > 0:
      rv = heappop(queue)
      queue_set.remove(rv)
      rv = -rv
      if rv in self.obs_rv_set:
        i = self.obs_rv2idx[rv]
        node_input = x[:, i].squeeze().item()
        shape = (batch_size, S, max(1, self.num_parents[rv]))
        node_input = torch.ones(shape) * node_input
      else:
        node_input = self.concat_inputs(node_inputs[rv])
      node_output = self.nodes[rv](node_input)
      factor = self.factors[rv]
      if factor.factor_type in ('SAME', 'INV'):
        out = self.extract_single_output(node_output)
        child_rv = factor.input_rvs[0]
        node_inputs[child_rv].append(out)
      elif factor.factor_type == 'AND':
        out1, out2 = self.extract_double_output(node_output)
        child1, child2 = factor.input_rvs
        node_inputs[child1].append(out1)
        node_inputs[child2].append(out2)
      elif factor.factor_type == 'PRIOR':
        i = self.prediction_rv2idx(rv)
        if i:
          msg_prediction[:, i] = node_output

      if node_inputs[rv]:
        del node_inputs[rv]

      for child in factor.input_rvs:
        child = -child
        if child not in queue_set:
          heappush(queue, child)
          queue_set.add(child)

    print('Forward pass completed in %.2f s' % (time() - start))
    return msg_prediction

  def prediction_rv2idx(self, rv_idx):
    assert rv_idx < self.num_input_bits
    return rv_idx

  def concat_inputs(self, list_of_inputs, debug=False):
    batch_size = list_of_inputs[0].size()[0]
    result = torch.zeros((batch_size, S, len(list_of_inputs)))
    for i, inp in enumerate(list_of_inputs):
      result[:, :, i] = inp
    return result

  def extract_single_output(self, x):
    return x[:, :, 0]

  def extract_double_output(self, x):
    return x[:, :, 0], x[:, :, 1]

  def make_node(self, factor):
    ident = factor.output_rv
    ftype = factor.factor_type
    num_inputs = self.num_parents[factor.output_rv]
    if ftype == 'AND':
      return AndNode(ident, num_inputs)
    elif ftype == 'INV':
      return InvNode(ident, num_inputs)
    elif ftype == 'SAME':
      return SameNode(ident, num_inputs)
    elif ftype == 'PRIOR':
      return PriorNode(ident, num_inputs)
