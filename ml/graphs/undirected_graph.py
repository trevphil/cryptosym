# -*- coding: utf-8 -*-
import networkx as nx
from time import time
import matplotlib.pyplot as plt
from multiprocessing import Pool

from utils.log import getLogger
from utils.constants import BIT_PRED, HASH_INPUT_NBITS, MAX_CONNECTIONS_PER_NODE


class UndirectedGraph(object):

  def __init__(self, prob, size, config, fc_graph=None,
               max_connections=MAX_CONNECTIONS_PER_NODE):
    self.logger = getLogger('undirected_graph')
    self.prob = prob
    self.num_workers = config.num_workers
    self.max_connections = max_connections
    self.N, self.n = size  # (number of samples, number of variables)
    if fc_graph is None:
      self.fc_graph = self.createFullyConnectedGraph()
    else:
      self.fc_graph = nx.read_yaml(fc_graph)
    self.graph = self.pruneGraph(self.fc_graph)


  def saveFullyConnectedGraph(self, filename):
    self.logger.info('Saving fully connected graph as "%s"...' % filename)
    nx.write_yaml(self.fc_graph, filename)


  def saveUndirectedGraph(self, filename):
    self.logger.info('Saving undirected Bayesian network as "%s"...' % filename)
    nx.write_yaml(self.graph, filename)


  def visualizeGraph(self, img_file):
    self.logger.info('Visualizing undirected Bayesian network...')

    plt.close()
    nx.draw_spectral(self.graph, with_labels=True)
    plt.savefig(img_file)


  def createFullyConnectedGraph(self):
    graph = nx.Graph()

    counter = 0
    max_count = (self.n * (self.n - 1) - HASH_INPUT_NBITS * (HASH_INPUT_NBITS - 1)) // 2

    self.logger.info('Calculating %d mutual information scores...' % max_count)
    start = time()

    for i in range(self.n):
      # (i, j) score is symmetric to (j, i) score so we only need to
      # calculate an upper-triangular matrix with zeros on the diagonal
      l_bound = 256
      u_bound = 256 + HASH_INPUT_NBITS
      inputs = [(self.prob, i, j) for j in range(i + 1, self.n)
                if not ((l_bound <= i < u_bound) and (l_bound <= j < u_bound))]

      with Pool(self.num_workers) as p:
        outputs = p.map(multiprocessingHelperFunc, inputs)
      
      for j, score in outputs:
        graph.add_edge(i, j, weight=score)
        counter += 1
        if counter % (max_count // 100) == 0:
          pct_done = 100.0 * counter / max_count
          self.logger.info('%.2f%% done.' % pct_done)
      
    self.logger.info('Finished calculating mutual information scores in %.1f sec.' % (time() - start))
    self.logger.info('The fully connected graph has %d edges.' % graph.number_of_edges())
    return graph


  def pruneGraph(self, fc_graph):
    graph = fc_graph.copy()

    prune = True
    while prune:
      prune = False
      for rv in graph.nodes():
        num_neighbors = len(graph[rv])
        if num_neighbors <= self.max_connections:
          continue

        # Prune away least negative edge
        neighbors = [n for n in graph.edges(rv, data='weight')]
        neighbors = list(sorted(neighbors, key=lambda n: n[2]))
        to_remove = neighbors[0] # first one has smallest mutual info score
        graph.remove_edge(to_remove[0], to_remove[1])
        prune = True

    components = [graph.subgraph(c) for c in nx.connected_components(graph)]
    relevant_component = [g for g in components if BIT_PRED in g.nodes()][0]

    msg = 'The optimized BN has %d edges.\n' % graph.number_of_edges()
    msg += '\tconnected = {}\n'.format(len(components) == 1)
    msg += '\tnum connected components = %d\n' % len(components)
    largest_cc = max(nx.connected_components(graph), key=len)
    msg += '\tlargest component has %d nodes\n' % len(largest_cc)
    msg += '\tcomponent with bit %d has %d nodes' % (BIT_PRED, relevant_component.number_of_nodes())
    self.logger.info(msg)

    return relevant_component


def multiprocessingHelperFunc(x):
  """
  A helper function for computing the fully-connected graph is given here
  at top-level due to pickling requirements of Python multiprocessing library
  """
  prob, i, j = x
  return j, prob.iHat([i, j])
