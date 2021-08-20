import networkx as nx
from networkx.drawing.nx_pydot import graphviz_layout
from matplotlib import pyplot as plt
from collections import defaultdict


def plot_dag(problem):
    directed_graph = nx.DiGraph()
    for gate in problem.gates:
        for inp in gate.inputs:
            directed_graph.add_edge(abs(inp), abs(gate.output))

    colors = ['#000000' for _ in range(problem.num_vars)]
    for input_idx in problem.input_indices:
        colors[abs(input_idx) - 1] = '#00ff00'
    for output_idx in problem.output_indices:
        colors[abs(output_idx) - 1] = '#ff0000'
    pos = graphviz_layout(directed_graph, prog='dot')
    nx.draw(directed_graph, pos, node_size=5,
            with_labels=False, node_color=colors, 
            width=0.25, arrowsize=4)
    # plt.savefig(viz_file)
    plt.show()


def plot_cnf(problem):
    """
    Create an edge between two variables if they appear in the same clause
    """
    n = problem.num_vars
    vars_per_clause = defaultdict(lambda: [])
    for lit_idx, clause_idx in problem.cnf_indices:
        if lit_idx >= n:
            var = lit_idx - n
        else:
            var = lit_idx
        vars_per_clause[clause_idx].append(var)
    
    g = nx.Graph()
    print('Adding edges...', end=' ')
    for _, v in vars_per_clause.items():
        for a in v:
            for b in v:
                if a != b:
                    g.add_edge(a, b)
    print('done.')

    print('Creating layout...', end=' ')
    pos = graphviz_layout(g, prog='dot')
    print('done.')
    
    print('Drawing...', end=' ')
    nx.draw(g, pos, node_size=5, with_labels=False,
            width=0.25)
    print('done.')

    plt.show()