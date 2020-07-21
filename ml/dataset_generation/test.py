# -*- coding: utf-8 -*-
import networkx as nx
from matplotlib import pyplot as plt

from dataset_generation import pseudo_hashes
from dataset_generation.factor import Factor
from dataset_generation.symbolic import SymBitVec, saveFactors
from dataset_generation.generate_dataset import sample


if __name__ == '__main__':
  sym_bv = SymBitVec(sample(32))
  hashed = pseudo_hashes.shiftRight(sym_bv)

  saveFactors('factors.txt')

  nx.draw(Factor.directed_graph, node_size=10, with_labels=False)
  plt.show()
