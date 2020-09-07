# -*- coding: utf-8 -*-

from time import time
import torch
import torch.nn.functional as F

class Loss(object):
    def __init__(self, factors, obs_rv_set, obs_rv2idx,
                             num_input_bits):
        self.factors = factors
        self.num_input_bits = num_input_bits
        self.obs_rv_set = obs_rv_set
        self.obs_rv2idx = obs_rv2idx

    def __call__(self, predicted_input, target_hash):
        start = time()
        node_val = dict()
        rvs = sorted(self.factors.keys())

        for rv in rvs:
            factor = self.factors[rv]
            ftype = factor.factor_type
            if ftype == 'PRIOR':
                assert rv < self.num_input_bits
                node_val[rv] = predicted_input[:, rv]
            elif ftype == 'SAME':
                node_val[rv] = node_val[factor.input_rvs[0]]
            elif ftype == 'INV':
                node_val[rv] = 1.0 - node_val[factor.input_rvs[0]]
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs
                node_val[rv] = node_val[inp1] * node_val[inp2]

        hash_out = torch.zeros((0, ))
        for rv in sorted(self.obs_rv_set):
            hash_out = torch.cat((hash_out, node_val[rv]))

        print('Loss computation finished in %.2f s' % (time() - start))
        return F.binary_cross_entropy_with_logits(
                hash_out, target_hash, reduction='mean')
