import os
import sys
import pygraphviz as pgv

class LogicGate(object):
  def __init__(self, t, output, inputs):
    self.t = t
    self.output = output
    self.inputs = inputs


def main(filename='/tmp/hash_symbols.txt'):
  I, O, N, M = (None, None, None, None)
  hash_inputs = None
  hash_outputs = None
  gates = []

  with open(filename, 'r') as f:
    for line in f:
      if line[0] == '#':
        continue

      parts = line.strip().split(' ')
      if len(parts) == 0:
        continue

      if None in (I, O, N, M):
        I, O, N, M = [int(s.strip()) for s in parts if len(s) > 0]
      elif hash_inputs is None:
        hash_inputs = [int(s.strip()) for s in parts if len(s) > 0]
      elif hash_outputs is None:
        hash_outputs = [int(s.strip()) for s in parts if len(s) > 0]
      else:
        gate_type = parts[0].strip()
        output = int(parts[1].strip())
        inputs = [int(s.strip()) for s in parts[2:] if len(s) > 0]
        gate = LogicGate(gate_type, output, inputs)
        gates.append(gate)

  def node_color(node):
    if node in hash_inputs:
      return 'green'
    elif node in hash_outputs:
      return 'red'
    else:
      return 'black'

  def edge_color(gate_type):
    if gate_type == 'A':
      return 'orange'
    elif gate_type == 'O':
      return 'blue'
    else:
      return 'black'

  graph = pgv.AGraph(strict=True, directed=True)
  graph.graph_attr.update(overlap=False, rank='same')
  graph.node_attr.update(width=0.2, height=0.2, margin=0)

  for i in range(1, N + 1):
    graph.add_node(i, color=node_color(i), shape='point')

  for gate in gates:
    for inp in gate.inputs:
      graph.add_edge(abs(inp), abs(gate.output), color=edge_color(gate.t))

  # neato, dot, twopi, circo, fdp, nop, wc, acyclic, gvpr, gvcolor, ccomps, sccmap, tred, sfdp, unflatten
  graph.layout(prog='sfdp')
  graph.draw('/tmp/graph.png')


if __name__ == '__main__':
  main()
