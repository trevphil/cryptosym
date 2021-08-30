import cv2
import dgl
import numpy as np
import networkx as nx
from networkx.drawing.nx_pydot import graphviz_layout
from matplotlib import pyplot as plt
from collections import defaultdict

from logic import ggnn_graph, mis


def get_dag(problem):
    dag = nx.DiGraph()
    for node in range(1, problem.num_vars + 1):
        dag.add_node(node)
    for gate in problem.gates:
        for inp in gate.inputs:
            dag.add_edge(abs(inp), abs(gate.output))
    return dag


def get_cnf(problem):
    g = mis.MaximumIndependentSet(problem, dict()).g
    return dgl.to_networkx(g)


def plot_dag(problem):
    dag = get_dag(problem)

    colors = ['#000000' for _ in range(problem.num_vars)]
    for input_idx in problem.input_indices:
        colors[abs(input_idx) - 1] = '#00ff00'
    for output_idx in problem.output_indices:
        colors[abs(output_idx) - 1] = '#ff0000'
    pos = graphviz_layout(dag, prog='dot')
    nx.draw(dag, pos, node_size=5,
            with_labels=False, node_color=colors, 
            width=0.25, arrowsize=4)
    # plt.savefig(viz_file)
    plt.show()


def plot_cnf(problem):
    g = get_cnf(problem)
    pos = graphviz_layout(g, prog='dot')
    nx.draw(g, pos, node_size=5, with_labels=False,
            width=0.25)
    plt.show()


def plot_loss_opencv(dag, cnf, loss_per_output, wait=1):
    fig, axes = plt.subplots(1, 2)
    fig.set_size_inches((10, 5))

    assert (dag.number_of_nodes() * 2) == cnf.number_of_nodes()    
    errors = [0.0 for _ in range(dag.number_of_nodes())]
    for var, loss in loss_per_output.items():
        errors[abs(var) - 1] += loss.item()
    cmap = 'jet'
    ns = 130

    dag_pos = graphviz_layout(dag, prog='dot')
    nx.draw(dag, dag_pos, axes[0], node_size=ns, node_color=errors,
            with_labels=True, width=0.7, arrowsize=8, cmap=cmap,
            font_color='white')
    cnf_pos = graphviz_layout(cnf, prog='dot')
    nx.draw(cnf, cnf_pos, axes[1], node_size=ns, node_color=errors + errors,
            with_labels=True, width=0.7, cmap=cmap, font_color='white')

    fig.canvas.draw()
    im = np.fromstring(fig.canvas.tostring_rgb(), dtype=np.uint8, sep='')
    im = im.reshape(fig.canvas.get_width_height()[::-1] + (3, ))
    im = cv2.cvtColor(im, cv2.COLOR_RGB2BGR)
    h, w = im.shape[:2]
    cv2.line(im, (w // 2, 0), (w // 2, h), (0, 0, 0))
    cv2.imshow('Errors', im)
    cv2.waitKey(wait)
    plt.close()
