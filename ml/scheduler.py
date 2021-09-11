import numpy as np

from logic.cnf import CNF
from logic.mis import MaximumIndependentSet
from generator import ProblemGenerator


class ProblemScheduler(object):
    def __init__(self):
        self.gen = ProblemGenerator()
        self.num_inputs = 8
        self.num_outputs = 8
        self.num_gates = 8
        self.problem = None
        self.bits = None
        self.observed = None
        self.mis = None
        self.base_cnf = None
        self.cnf = None
        self.label = None

        self.num_solved_for_current_problem = 0
        self.step_problem(True)
    
    def reset(self):
        self.num_inputs = 8
        self.num_outputs = 8
        self.num_gates = 8
        self.step_problem(True)

    def step_problem(self, create_new_problem):
        if create_new_problem or self.problem is None:
            self.problem = self.gen.create_problem(
                    self.num_inputs, self.num_outputs, self.num_gates)
            self.base_cnf = CNF(self.problem)
            self.num_solved_for_current_problem = 0

        self.bits, self.observed = self.problem.random_bits()
        self.cnf = self.base_cnf.simplify(self.observed)
        assert self.cnf is not None, 'CNF is UNSAT after simplification!'
        self.mis = MaximumIndependentSet(self.cnf)
        self.label = self.mis.cnf_to_mis_solution(self.bits)
        assert self.label is not None, 'Solution conversion SAT --> MIS failed'
    
    def increase_difficulty(self):
        self.num_inputs = min(self.num_inputs + 1, 256)
        self.num_outputs = min(self.num_outputs + 1, 256)
        min_gates = max(self.num_inputs, self.num_outputs)
        max_gates = int(2 * min_gates)
        self.num_gates += np.random.randint(1, 4)
        self.num_gates = min(max_gates, max(min_gates, self.num_gates))

    def record_train_result(self, solved):
        if solved:
            self.num_solved_for_current_problem += 1
            if self.num_solved_for_current_problem >= 64:
                self.increase_difficulty()
                self.step_problem(True)  # New, more difficult problem
            else:
                self.step_problem(False)  # Same problem, new inputs
        else:
            self.step_problem(False)  # Same problem, new inputs
