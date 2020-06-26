# -*- coding: utf-8 -*-
import numpy as np
import networkx as nx
import matplotlib.pyplot as plt

from utils.log import getLogger


class UndirectedGraph(object):

  def __init__(self, config):
    self.config = config
    self.logger = getLogger('undirected_graph')
    self.logger.info('Loading undirected graph...')
    self.graph = nx.from_numpy_matrix(np.loadtxt(config.graph, dtype=bool, delimiter=','))
    self.logger.info('Finished loading undirected graph. Performing post-processing...')
    self.graph = self.postProcess(self.graph)
    self.logger.info('Finished post-processing of undirected graph.')


  def visualizeGraph(self, img_file):
    self.logger.info('Visualizing undirected Bayesian network...')

    plt.close()
    nx.draw_shell(self.graph, with_labels=False, width=0.1, node_size=5)
    plt.savefig(img_file)


  def postProcess(self, graph):
    bit_pred = self.config.bit_pred
    components = [graph.subgraph(c) for c in nx.connected_components(graph)]
    relevant_component = [g for g in components if bit_pred in g.nodes()][0]

    msg = 'The optimized BN has %d edges.\n' % graph.number_of_edges()
    msg += '\tconnected = {}\n'.format(len(components) == 1)
    msg += '\tnum connected components = %d\n' % len(components)
    largest_cc = max(nx.connected_components(graph), key=len)
    msg += '\tlargest component has %d nodes\n' % len(largest_cc)
    msg += '\tcomponent with bit %d has %d nodes\n' % (bit_pred, relevant_component.number_of_nodes())
    msg += '\tbit %d is connected to %s' % (bit_pred, list(sorted(relevant_component[bit_pred].keys())))
    self.logger.info(msg)

    return relevant_component
