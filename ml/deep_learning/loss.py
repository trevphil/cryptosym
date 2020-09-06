# -*- coding: utf-8 -*-

import torch.nn.functional as F

class Loss(object):
  def __init__(self):
    pass

  def __call__(self, prediction, target):
    return F.binary_cross_entropy(prediction, target, reduction='sum')
