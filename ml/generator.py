import random
from copy import deepcopy
from problem import Problem, AndGate
from collections import defaultdict

class ProblemGenerator(object):
  def __init__(self, opts):
    self.opts = opts

  def rand_flip(self, x):
    return x * (1 if random.random() < 0.5 else -1)

  def create_problem(self, input_size, output_size, num_gates):
    pool = []
    gates = []
    assignments = dict()
    seen = set()
    leaves = set()

    # Assign random values to inputs of the hash function
    for var in range(1, input_size + 1):
      assignments[var] = int(random.random() < 0.5)
      literal = self.rand_flip(var)
      pool.append(literal)
      leaves.add(literal)

    max_var = input_size + 1

    for _ in range(num_gates):
      # Avoid duplicating the selection of AND-gate inputs
      inp1 = random.sample(leaves, k=1)[0]
      inp2 = random.sample(pool, k=1)[0]
      while (inp1 == inp2) or ((inp1, inp2) in seen):
        inp1 = random.sample(leaves, k=1)[0]
        inp2 = random.sample(pool, k=1)[0]

      # Add these AND-gate input to the set of "used" input combinations
      seen.add((inp1, inp2))
      seen.add((inp2, inp1))

      # Remove selected inputs from directed graph "leaf" set if necessary
      leaves.remove(inp1)
      if inp2 in leaves:
        leaves.remove(inp2)

      gates.append(AndGate(max_var, inp1, inp2))
      literal = self.rand_flip(max_var)
      pool.append(literal)
      leaves.add(literal)

      # Compute the output bit of the AND gate
      inp1_val = assignments[abs(inp1)]
      if inp1 < 0:
        inp1_val = 1 - inp1_val
      inp2_val = assignments[abs(inp1)]
      if inp2 < 0:
        inp2_val = 1 - inp2_val
      assignments[max_var] = inp1_val * inp2_val
      max_var += 1

    input_indices = list(range(1, input_size + 1))
    if output_size >= len(pool):
      output_indices = pool
    else:
      output_indices = pool[-output_size:]

    num_vars = (max_var - 1)
    cnf_indices = []
    cols_per_row = defaultdict(lambda: set())

    def add_clause(lits, ci):
      for lit in lits:
        if lit < 0:
          row = abs(lit) - 1 + num_vars
          cnf_indices.append((row, ci))
          cols_per_row[row].add(ci)
        else:
          row = abs(lit) - 1
          cnf_indices.append((row, ci))
          cols_per_row[row].add(ci)
      return ci + 1

    clause_idx = 0
    for g in gates:
      clause_idx = add_clause([g.inp1, -g.out], clause_idx)
      clause_idx = add_clause([g.inp2, -g.out], clause_idx)
      clause_idx = add_clause([-g.inp1, -g.inp2, g.out], clause_idx)
    num_clauses = clause_idx
    
    # Add random connections!
    sparsity = 0.1
    for row in range(int(2 * num_vars)):
      existing_cols = deepcopy(cols_per_row[row])
      k = int((sparsity - len(existing_cols) / num_clauses) * num_clauses + 0.5)
      while k > 0:
        col = None
        while (col is None) or (col in existing_cols):
          col = random.randrange(0, num_clauses)
        existing_cols.add(col)
        cnf_indices.append((row, col))
        k -= 1

    problem = Problem(input_size=input_size,
                      output_size=output_size,
                      num_vars=num_vars,
                      num_gates=len(gates),
                      num_clauses=num_clauses,
                      input_indices=input_indices,
                      output_indices=output_indices,
                      cnf_indices=cnf_indices,
                      and_gates=gates)

    # Cache the values of the hash output bits
    observed = dict()
    for out_idx in output_indices:
      observed[abs(out_idx)] = assignments[abs(out_idx)]
    problem.set_observed(observed)

    return problem

if __name__ == '__main__':
  from time import time
  import networkx as nx
  from networkx.drawing.nx_pydot import graphviz_layout
  from matplotlib import pyplot as plt
  
  from config import HashSATConfig
  
  opts = HashSATConfig()
  gen = ProblemGenerator(opts)
  
  input_size = 16
  output_size = 16
  num_gates = 128

  start = time()
  problem = gen.create_problem(input_size, output_size, num_gates)
  runtime_ms = (time() - start) * 1000.0
  print(f'Generated problem in {runtime_ms} ms')
  print(f'\tin={input_size}, out={output_size}, gates={num_gates}')

  directed_graph = nx.DiGraph()
  for gate in problem.and_gates:
    directed_graph.add_edge(abs(gate.inp1), abs(gate.out))
    directed_graph.add_edge(abs(gate.inp2), abs(gate.out))

  colors = ['#000000' for _ in range(problem.num_vars)]
  for input_idx in problem.input_indices:
    colors[abs(input_idx) - 1] = '#00ff00'
  for output_idx in problem.output_indices:
    colors[abs(output_idx) - 1] = '#ff0000'
  pos = graphviz_layout(directed_graph, prog='dot')
  nx.draw(directed_graph, pos, node_size=5,
          with_labels=False, node_color=colors, width=0.25, arrowsize=4)
  # plt.savefig(viz_file)
  plt.show()
