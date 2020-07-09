# -*- coding: utf-8 -*-
#!/usr/bin/python3

"""
This is a script to visualize the results of the log-likelihood ratio
of each vertex in the undirected graph during loopy belief propagation in C++.
"""

import networkx as nx
from matplotlib import pyplot as plt
from matplotlib import animation


def animate(i, fig):
  print('Visualizing iteration %d' % i)
  g = nx.read_graphml('./cpp_code/viz/grap_viz_%d.xml' % i)
  components = [g.subgraph(c) for c in nx.connected_components(g)]
  components = list(sorted(components, key=lambda x: x.number_of_nodes()))
  g = components[-1]

  node_colors = []
  for _, data in g.nodes(data=True):
    node_colors.append(data['weight'])

  # https://networkx.github.io/documentation/stable/reference/generated/networkx.drawing.nx_pylab.draw_networkx.html#networkx.drawing.nx_pylab.draw_networkx
  fig.clf()
  pos = nx.random_layout(g, seed=0)
  nx.draw(g, pos=pos, node_color=node_colors, with_labels=False, node_size=15, width=0.5)


if __name__ == '__main__':
  fig = plt.figure()
  def anim(i):
    return animate(i, fig)

  anim = animation.FuncAnimation(fig, anim, frames=30, interval=20, blit=False)
  anim.save('viz.mp4', fps=2)  # Need ffmpeg (`brew install ffmpeg`)
