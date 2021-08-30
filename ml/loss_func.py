import torch
from collections import defaultdict

from logic.gate import GateType, gate_type_to_str


def _get_val(pred_bits, literal):
    idx = abs(literal) - 1
    bit = pred_bits[idx]
    if literal < 0:
        bit = 1 - bit
    return bit


class PreimageLoss(object):
    def __init__(self, mis):
        self.mis = mis
        self.bce_fn = torch.nn.BCELoss(reduction='sum')

    def bce(self, pred_solutions, label):
        label = label.float()
        num_solutions = pred_solutions.size(1)
        min_loss = None

        for i in range(num_solutions):
            l = self.bce_fn(pred_solutions[:, i], label)
            if min_loss is None or l < min_loss:
                min_loss = l

        return min_loss

    def get_solution(self, pred_solutions):
        num_solutions = pred_solutions.size(1)
        pred_solutions = torch.round(pred_solutions).int()

        for sol_idx in range(num_solutions):
            node_labeling = pred_solutions[:, sol_idx]
            bits = self.mis.mis_to_cnf_solution(node_labeling,
                                                conflict_is_error=False)
            if bits is None:
                continue  # A variable was assigned to be both 0 and 1

            assignments = dict()
            for bit_idx in range(bits.size(0)):
                lit = bit_idx + 1
                val = bits[bit_idx].item()
                assignments[lit] = val
                assignments[-lit] = 1 - val

            if self.mis.cnf.is_sat(assignments):
                return assignments

        return None  # None of the solutions were valid
