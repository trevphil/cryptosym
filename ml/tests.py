import os
import random
from time import time
import torch
import numpy as np
from collections import defaultdict
from termcolor import cprint

from logic.gate import (
    LogicGate,
    GateType,
    needs_gradient_friendly,
    inputs_for_gate,
    gate_type_to_str,
)
from logic.problem import Problem
from logic.cnf import CNF
from logic.mis import MaximumIndependentSet
from logic.ggnn_graph import problem_to_ggnn_graph
from generator import ProblemGenerator
from models.gated_graph_conv import HashSAT
from loss_func import PreimageLoss
from opts import PreimageOpts


class RuntimeTracker(object):
    def __init__(self):
        self.cum_runtime = defaultdict(lambda: 0.0)
        self.count = defaultdict(lambda: 0)

    def record(self, name, runtime_ms):
        self.cum_runtime[name] += runtime_ms
        self.count[name] += 1

    def print_runtimes(self):
        if len(self.cum_runtime) == 0:
            return

        runtimes = []
        for name, cum_t in self.cum_runtime.items():
            mean_runtime = cum_t / self.count[name]
            runtimes.append((name, mean_runtime))
        runtimes = list(sorted(runtimes, key=lambda tup: -tup[1]))

        cprint("RUNTIME", "yellow")
        for name, runtime_ms in runtimes:
            s = "{:<45}".format(name) + ("%.1f ms" % runtime_ms)
            cprint(s, "yellow")


class TimeIt(object):
    def __init__(self, tracker, name):
        self.tracker = tracker
        self.name = name

    def __enter__(self):
        self.start = time()

    def __exit__(self, type, value, traceback):
        runtime_ms = (time() - self.start) * 1000.0
        self.tracker.record(self.name, runtime_ms)


class TestSuite(object):
    def __init__(self, seed=10, test_sha256=False):
        self.tracker = RuntimeTracker()
        self.test_sha256 = test_sha256
        seed = 10
        random.seed(seed)
        np.random.seed(seed)
        torch.manual_seed(seed)

    def verify(self, cond, err_msg=None):
        if not cond:
            if isinstance(err_msg, str):
                cprint(err_msg, "red")
            assert False, f"{err_msg}"

    def run(self):
        self.test_logic_gates()

        if self.test_sha256:
            sha_problem, sha_bits, sha_obs = self.test_sha256_problem()
            cprint("Loaded SHA256 problem", "green")
            sha_cnf = self.test_sha256_cnf(sha_problem, sha_bits, sha_obs)
            cprint("CNF tests passed for SHA256 problem", "green")
            ggnn = self.test_sha256_to_ggnn(sha_problem, sha_bits, sha_obs)
            cprint("Conversion to GGNN graph worked", "green")

        N = 100
        for _ in range(N):
            problem, bits, observed = self.test_generator()
            mis, label = self.test_mis(problem, bits, observed)
            # self.test_local_search(mis, label)
        cprint("MIS conversions worked", "green")

        for _ in range(N):
            model, pred = self.test_model(mis.g)
            loss_fn, loss, sol = self.test_loss(mis, pred, label)
        cprint("NN model and loss function worked", "green")

        self.tracker.print_runtimes()
        cprint("All tests passed", "green")

    def test_logic_gates(self):
        t = torch.zeros(10, requires_grad=True)
        assert needs_gradient_friendly([t])
        assert needs_gradient_friendly((t,))
        assert not needs_gradient_friendly([1, 0, 1])

        gate_computation = {
            GateType.and_gate: (lambda x: x[0] & x[1]),
            GateType.xor_gate: (lambda x: x[0] ^ x[1]),
            GateType.or_gate: (lambda x: x[0] | x[1]),
            GateType.maj_gate: (lambda x: (1 if sum(x) > 1 else 0)),
            GateType.xor3_gate: (lambda x: x[0] ^ x[1] ^ x[2]),
        }

        for gate_type in GateType:
            gate_name = gate_type_to_str(gate_type)
            num_inputs = inputs_for_gate(gate_type)
            inputs = list(range(1, num_inputs + 1))
            output = num_inputs + 1
            gate = LogicGate(gate_type, output=output, inputs=inputs, depth=0)
            cnf = gate.cnf_clauses()

            for i in range(1 << num_inputs):
                input_values = [(i >> x) & 1 for x in range(num_inputs)]
                expected = gate_computation[gate_type](input_values)
                v1 = gate.compute_output(input_values)
                v2 = gate.compute_output_gradient_friendly(input_values)
                binstr = format(i, f"0{num_inputs}b")
                msg = f"Testing {gate_name}: {binstr} --> {(expected, v1, v2)}"

                self.verify(expected == v1, msg)
                self.verify(expected == v2, msg)

                # Validate the CNF formula
                assignments = {inputs[i]: input_values[i] for i in range(num_inputs)}

                # Ensure formula is satisfied if gate(inputs) = expected_output
                assignments[output] = expected
                num_sat_clauses = 0
                for clause in cnf:
                    for lit in clause:
                        val = int(assignments[abs(lit)])
                        if lit < 0:
                            val = 1 - val
                        if val:
                            num_sat_clauses += 1
                            break
                self.verify(num_sat_clauses == len(cnf))

                # Ensure formula is NOT satisfied if gate(inputs) = wrong_output
                assignments[output] = 1 - int(expected)
                num_sat_clauses = 0
                for clause in cnf:
                    for lit in clause:
                        val = int(assignments[abs(lit)])
                        if lit < 0:
                            val = 1 - val
                        if val:
                            num_sat_clauses += 1
                            break
                self.verify(num_sat_clauses < len(cnf))

    def test_sha256_problem(self):
        sym_filename = os.path.join("samples", "sha256_d64_sym.txt")

        with TimeIt(self.tracker, "SHA256 problem loading") as t:
            problem = Problem.from_file(sym_filename)

        with TimeIt(self.tracker, "SHA256 forward computation") as t:
            bits, observed = problem.random_bits()

        return problem, bits, observed

    def test_sha256_cnf(self, problem, bits, observed):
        with TimeIt(self.tracker, "SHA256 conversion of problem to CNF") as t:
            cnf = CNF(problem)

        nc_before = len(cnf.clauses)

        with TimeIt(self.tracker, "SHA256 CNF simplification") as t:
            cnf = cnf.simplify(observed)

        self.verify(cnf is not None, "simplify() returned None, i.e. problem is UNSAT")
        nc_after = len(cnf.clauses)
        self.verify(nc_after <= nc_before, "simplify() should not increase # clauses")

        for i, clause in enumerate(cnf.clauses):
            msg = f"Clause {i} = {clause} but simplification should remove 0-SAT 1-SAT"
            self.verify(len(clause) > 1, msg)

        assignments = dict()
        for bit_idx in range(0, problem.num_vars):
            lit = bit_idx + 1
            assignments[lit] = int(bits[bit_idx].item())
            assignments[-lit] = 1 - assignments[lit]

        with TimeIt(self.tracker, "SHA256 checking if CNF is SAT") as t:
            sat = cnf.is_sat(assignments)

        self.verify(sat, "CNF should be SAT with correct variable assignments")

        for inp in problem.input_indices:
            if inp == 0:
                continue

            true_assignment = assignments[inp]
            assignments[inp] = 1 - true_assignment
            assignments[-inp] = true_assignment

            with TimeIt(self.tracker, "SHA256 checking if CNF is UNSAT") as t:
                sat = cnf.is_sat(assignments)
            self.verify(not sat, "CNF should be UNSAT with a bad input assignment")

            assignments[inp] = true_assignment
            assignments[-inp] = 1 - true_assignment

        return cnf

    def test_sha256_to_ggnn(self, problem, bits, observed):
        with TimeIt(self.tracker, "SHA256 conversion of problem to GGNN") as t:
            g = problem_to_ggnn_graph(problem, observed)

        self.verify(g is not None, "GGNN graph is None")
        return g

    def test_generator(self):
        gen = ProblemGenerator()

        input_size = 128
        output_size = 128
        num_gates = 256

        with TimeIt(self.tracker, "Problem generation") as t:
            problem = gen.create_problem(input_size, output_size, num_gates)

        with TimeIt(self.tracker, "Problem forward computation") as t:
            bits, observed = problem.random_bits()

        return problem, bits, observed

    def test_mis(self, problem, bits, observed):
        with TimeIt(self.tracker, "Conversion of problem to CNF") as t:
            cnf = CNF(problem)

        with TimeIt(self.tracker, "CNF simplification") as t:
            cnf = cnf.simplify(observed)

        self.verify(cnf is not None, "CNF simplification resulted in UNSAT")

        assignments = dict()
        for bit_idx in range(0, problem.num_vars):
            lit = bit_idx + 1
            assignments[lit] = int(bits[bit_idx].item())
            assignments[-lit] = 1 - assignments[lit]

        with TimeIt(self.tracker, "Checking if CNF is SAT") as t:
            sat = cnf.is_sat(assignments)
        self.verify(sat, "Sanity check failed! CNF should be SAT")

        with TimeIt(self.tracker, "Constructing MIS from CNF") as t:
            mis = MaximumIndependentSet(cnf=cnf)

        with TimeIt(self.tracker, "CNF bits to MIS node labeling") as t:
            label = mis.cnf_to_mis_solution(bits)
        self.verify(isinstance(label, torch.Tensor), "CNF --> MIS is UNSAT")

        with TimeIt(self.tracker, "Check if label is independent set") as t:
            is_indep_set = mis.is_independent_set(label)
        self.verify(is_indep_set, "MIS solution is not independent set")

        with TimeIt(self.tracker, "Check if MIS node label is optimal (SAT)") as t:
            sat = mis.is_sat(label)
        self.verify(sat, "MIS solution does not solve SAT problem")

        # Violated independent set constraints, check that it is detected
        bad_label = label.clone()
        for i in range(bad_label.size(0)):
            if bad_label[i] == 0:
                bad_label[i] = 1
                break

        with TimeIt(self.tracker, "Check if label is independent set") as t:
            is_indep_set = mis.is_independent_set(bad_label)
        self.verify(not is_indep_set, "MIS solution should not be independent set")

        with TimeIt(self.tracker, "Check if MIS node label is optimal (SAT)") as t:
            sat = mis.is_sat(bad_label)
        self.verify(not sat, "MIS solution does not solve SAT problem")

        with TimeIt(self.tracker, "Convert from MIS label to SAT bits") as t:
            remapped_bits = mis.mis_to_cnf_solution(label)

        assignments = dict()
        for bit_idx in range(remapped_bits.size(0)):
            lit = bit_idx + 1
            assignments[lit] = int(remapped_bits[bit_idx].item())
            assignments[-lit] = 1 - assignments[lit]

        with TimeIt(self.tracker, "Checking if CNF is SAT") as t:
            sat = cnf.is_sat(assignments)
        self.verify(sat, "MIS --> CNF is UNSAT")

        return mis, label

    def test_local_search(self, mis, node_labeling):
        self.verify(False, "TODO")

    def test_model(self, g):
        num_layers = 20
        hidden_size = 32
        num_solutions = 64
        model = HashSAT(
            num_layers=num_layers, hidden_size=hidden_size, num_solutions=num_solutions
        )

        with TimeIt(self.tracker, "Model forward pass") as t:
            pred = model(g)

        shape = pred.shape
        self.verify(shape[0] == g.num_nodes(), f"Model output: {shape}")
        self.verify(shape[1] == num_solutions, f"Model output: {shape}")

        return model, pred

    def test_loss(self, mis, pred, label):
        loss_fn = PreimageLoss()

        with TimeIt(self.tracker, "BCE loss computation") as t:
            loss = loss_fn.bce(pred, label)
        self.verify(torch.isfinite(loss), f"Loss = {loss}")

        with TimeIt(self.tracker, "Searching for MIS in model output") as t:
            sol = loss_fn.get_solution(pred, mis)

        return loss_fn, loss, sol


if __name__ == "__main__":
    test_suite = TestSuite()
    test_suite.run()
