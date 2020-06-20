# -*- coding: utf-8 -*-
import networkx as nx
import matplotlib.pyplot as plt

from utils.constants import BIT_PRED, MAX_CONNECTIONS_PER_NODE


class UndirectedGraph(object):

  def __init__(self, prob, size, max_connections=MAX_CONNECTIONS_PER_NODE,
               fc_graph=None, verbose=True):
    self.prob = prob
    self.verbose = verbose
    self.max_connections = max_connections
    self.N, self.n = size # (number of samples, number of variables)
    if fc_graph is None:
      self.fc_graph = self.createFullyConnectedGraph()
    else:
      self.fc_graph = nx.read_yaml(fc_graph)
    self.graph = self.pruneGraph(self.fc_graph)


  def saveFullyConnectedGraph(self, filename):
    if self.verbose:
      print('Saving fully connected graph as "%s"...' % filename)
    
    nx.write_yaml(self.fc_graph, filename)


  def saveUndirectedGraph(self, filename):
    if self.verbose:
      print('Saving undirected Bayesian network as "%s"...' % filename)

    nx.write_yaml(self.graph, filename)


  def visualizeGraph(self, img_file):
    if self.verbose:
      print('Visualizing undirected Bayesian network...')

    plt.close()
    nx.draw_spectral(self.graph, with_labels=True)
    plt.savefig(img_file)


  def createFullyConnectedGraph(self):
    if self.verbose:
      print('Calculating mutual information scores...')

    graph = nx.Graph()
    counter = 0
    max_count = self.n * (self.n - 1) / 2

    for i in range(self.n):
      # (i, j) score is symmetric to (j, i) score so we only need to
      # calculate an upper-triangular matrix with zeros on the diagonal
      for j in range(i + 1, self.n):
        counter += 1

        if i >= 256 and j >= 256:
          # No edges between hash input bit random variables
          break

        weight = self.prob.iHat([i, j])
        graph.add_edge(i, j, weight=weight)

        if self.verbose and counter % (max_count / 10) == 0:
          pct_done = 100.0 * counter / max_count
          print('%.2f%% done.' % pct_done)

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

    if self.verbose:
      print('The optimized BN has %d edges.' % graph.number_of_edges())
      print('\tconnected = {}'.format(len(components) == 1))
      print('\tnum connected components = %d' % len(components))
      largest_cc = max(nx.connected_components(graph), key=len)
      print('\tlargest component has %d nodes' % len(largest_cc))
      print('\tcomponent with bit %d has %d nodes' % (BIT_PRED, relevant_component.number_of_nodes()))

    return relevant_component
