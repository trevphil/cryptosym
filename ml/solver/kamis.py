import os
import ctypes
import torch


class KaMIS(object):
    def __init__(self):
        dir_path = os.path.dirname(os.path.realpath(__file__))
        self.lib = ctypes.CDLL("%s/libreduce.so" % dir_path)

    def _ctype_graph(self, g):
        nn = g.num_nodes()
        ne = g.num_edges()
        e_list_from = (ctypes.c_int * ne)()
        e_list_to = (ctypes.c_int * ne)()

        if ne > 0:
            a, b = g.edges()
            e_list_from[:] = a.numpy()
            e_list_to[:] = b.numpy()

        e_from = ctypes.cast(e_list_from, ctypes.c_void_p)
        e_to = ctypes.cast(e_list_to, ctypes.c_void_p)
        return nn, ne, e_from, e_to

    def reduce_graph(self, g):
        n_nodes, n_edges, e_froms, e_tos = self._ctype_graph(g)
        reduced_node = (ctypes.c_int * n_nodes)()
        new_n_nodes = ctypes.c_int()
        new_n_edges = ctypes.c_int()
        reduced_xadj = (ctypes.c_int * (n_nodes + 1))()
        reduced_adjncy = (ctypes.c_int * (2 * n_edges))()
        mapping = (ctypes.c_int * n_nodes)()
        reverse_mapping = (ctypes.c_int * n_nodes)()
        crt_is_size = self.lib.Reduce(
            n_nodes,
            n_edges,
            e_froms,
            e_tos,
            reduced_node,
            ctypes.byref(new_n_nodes),
            ctypes.byref(new_n_edges),
            reduced_xadj,
            reduced_adjncy,
            mapping,
            reverse_mapping,
        )
        new_n_nodes = new_n_nodes.value
        reduced_node = torch.as_tensor(reduced_node[:])
        reverse_mapping = torch.as_tensor(reverse_mapping[:])
        reverse_mapping = reverse_mapping[:new_n_nodes]
        return new_n_nodes, reduced_node, reverse_mapping

    def local_search(self, g, indset):
        n_nodes, n_edges, e_froms, e_tos = self._ctype_graph(g)
        init_mis = (ctypes.c_int * n_nodes)()
        final_mis = (ctypes.c_int * n_nodes)()
        init_mis[:] = indset.numpy()
        init_mis = ctypes.cast(init_mis, ctypes.c_void_p)
        self.lib.LocalSearch(n_nodes, n_edges, e_froms, e_tos, init_mis, final_mis)
        return torch.as_tensor(final_mis[:])

    def test(self):
        self.lib.test()
