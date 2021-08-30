import os
import random
from time import time
import torch
import numpy as np

from logic.gate import (
    LogicGate,
    GateType,
    needs_gradient_friendly,
    inputs_for_gate,
    gate_type_to_str
)
from logic.problem import Problem
from logic.cnf import CNF
from logic.mis import MaximumIndependentSet
from logic.ggnn_graph import problem_to_ggnn_graph
from generator import ProblemGenerator
from models.hashsat import HashSAT
from loss_func import PreimageLoss
from opts import PreimageOpts


def test_logic_gates():
    t = torch.zeros(10, requires_grad=True)
    assert needs_gradient_friendly([t])
    assert needs_gradient_friendly((t, ))
    assert not needs_gradient_friendly([1, 0, 1])

    gate_computation = {
        GateType.and_gate: (lambda x: x[0] & x[1]),
        GateType.xor_gate: (lambda x: x[0] ^ x[1]),
        GateType.or_gate: (lambda x: x[0] | x[1]),
        GateType.maj_gate: (lambda x: (1 if sum(x) > 1 else 0)),
        GateType.xor3_gate: (lambda x: x[0] ^ x[1] ^ x[2])
    }

    for gate_type in GateType:
        print(f'\nLogic gate: {gate_type_to_str(gate_type)}')
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
            binstr = format(i, f'0{num_inputs}b')
            print(f'Testing input: {binstr} --> {(expected, v1, v2)}')
            assert expected == v1
            assert expected == v2

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
            assert num_sat_clauses == len(cnf)

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
            assert num_sat_clauses < len(cnf)


def test_problem():
    sym_filename = os.path.join('samples', 'sha256_d64_sym.txt')
    print(f'Problem: loading problem: {sym_filename}')

    start = time()
    problem = Problem.from_file(sym_filename)
    runtime_ms = (time() - start) * 1000.0
    print(f'Problem: loaded problem in {runtime_ms:.1f} ms')

    start = time()
    bits, observed = problem.random_bits()
    runtime_ms = (time() - start) * 1000.0
    print(f'Problem: forward computation in {runtime_ms:.1f} ms')

    return problem, bits, observed


def test_cnf(problem, bits, observed):
    start = time()
    cnf = CNF(problem)
    runtime_ms = (time() - start) * 1000.0
    print(f'CNF: converted problem to CNF in {runtime_ms:.1f} ms')

    start = time()
    nc_before = cnf.num_clauses
    cnf = cnf.simplify(observed)
    assert cnf is not None, 'simplify() returned None, i.e. problem is UNSAT'
    nc_after = cnf.num_clauses
    assert nc_after <= nc_before
    runtime_ms = (time() - start) * 1000.0
    print(f'CNF: simplified in {runtime_ms:.1f} ms (-{nc_before - nc_after} clauses)')

    for i, clause in enumerate(cnf.clauses):
        msg = f'Clause {i} = {clause} but simplification should remove 0-SAT 1-SAT'
        assert len(clause) > 1, msg

    assignments = dict()
    for bit_idx in range(0, problem.num_vars):
        lit = bit_idx + 1
        assignments[lit] = int(bits[bit_idx].item())
        assignments[-lit] = 1 - assignments[lit]

    start = time()
    sat = cnf.is_sat(assignments)
    runtime_ms = (time() - start) * 1000.0
    print(f'CNF: SAT computed in {runtime_ms:.1f} ms (SAT = {sat})')
    assert sat, 'CNF should be SAT with correct variable assignments'

    for inp in problem.input_indices:
        if inp == 0:
            continue

        true_assignment = assignments[inp]
        assignments[inp] = 1 - true_assignment
        assignments[-inp] = true_assignment

        sat = cnf.is_sat(assignments)
        assert not sat, 'CNF should be UNSAT with a bad input assignment'

        assignments[inp] = true_assignment
        assignments[-inp] = 1 - true_assignment

    print('CNF: using any incorrect input value causes UNSAT result.')

    return cnf


def test_ggnn_conversion(problem, bits, observed):
    start = time()
    g = problem_to_ggnn_graph(problem, observed)
    runtime_ms = (time() - start) * 1000.0
    print(f'GGNN: conversion to GGNN graph in {runtime_ms:.1f} ms')
    return g


def test_generator():
    gen = ProblemGenerator()

    input_size = 128
    output_size = 128
    num_gates = 256

    start = time()
    problem = gen.create_problem(input_size, output_size, num_gates)
    runtime_ms = (time() - start) * 1000.0
    print(f'Generator: generated problem in {runtime_ms:.1f} ms')
    print(f' --> in={input_size}, out={output_size}, gates={num_gates}')

    bits, observed = problem.random_bits()
    return problem, bits, observed


def test_mis(problem, bits, observed):
    cnf = CNF(problem)
    cnf = cnf.simplify(observed)
    assert cnf is not None, 'CNF simplification resulted in UNSAT'

    assignments = dict()
    for bit_idx in range(0, problem.num_vars):
        lit = bit_idx + 1
        assignments[lit] = int(bits[bit_idx].item())
        assignments[-lit] = 1 - assignments[lit]
    assert cnf.is_sat(assignments), f'Sanity check failed\n{cnf}'

    start = time()
    mis = MaximumIndependentSet(cnf)
    runtime_ms = (time() - start) * 1000.0
    print('MIS: conversion to maximum independent set')
    print(f'\ttime:  {runtime_ms:.1f} ms')
    print(f'\tnodes: {mis.num_nodes}')
    print(f'\tedges: {mis.num_edges}')

    start = time()
    label = mis.cnf_to_mis_solution(bits)
    assert label is not None, 'CNF --> MIS is UNSAT'
    runtime_ms = (time() - start) * 1000.0
    print(f'MIS: remapped bits --> label in {runtime_ms:.1f} ms')

    start = time()
    remapped_bits = mis.mis_to_cnf_solution(label)
    runtime_ms = (time() - start) * 1000.0
    print(f'MIS: remapped label --> bits in {runtime_ms:.1f} ms')

    assignments = dict()
    for bit_idx in range(0, remapped_bits.size(0)):
        lit = bit_idx + 1
        assignments[lit] = int(remapped_bits[bit_idx].item())
        assignments[-lit] = 1 - assignments[lit]
    assert cnf.is_sat(assignments)

    return mis, label


def test_model(mis):
    num_layers = 20
    hidden_size = 32
    num_solutions = 64
    model = HashSAT(num_layers=num_layers,
                    hidden_size=hidden_size,
                    num_solutions=num_solutions)
    start = time()
    pred = model(mis.g)
    assert pred.size(0) == mis.num_nodes
    assert pred.size(1) == num_solutions
    runtime_ms = (time() - start) * 1000.0
    print(f'Model: completed forward pass in {runtime_ms:.1f} ms')

    return model, pred


def test_loss(mis, pred, label):
    loss_fn = PreimageLoss(mis)

    start = time()
    loss = loss_fn.bce(pred, label)
    runtime_ms = (time() - start) * 1000.0
    print(f'Loss: computed BCE loss = {loss:.2f} in {runtime_ms:.1f} ms')

    start = time()
    sol = loss_fn.get_solution(pred)
    runtime_ms = (time() - start) * 1000.0
    print(f'Loss: checked for solution in {runtime_ms:.1f} ms')
    print(f' --> solution = {sol}')

    return loss_fn, loss


if __name__ == '__main__':
    seed = 10
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)

    test_logic_gates()

    problem, bits, observed = test_problem()
    cnf = test_cnf(problem, bits, observed)
    ggnn_graph = test_ggnn_conversion(problem, bits, observed)

    for _ in range(100):
        problem, bits, observed = test_generator()
        mis, label = test_mis(problem, bits, observed)

    model, pred = test_model(mis)
    loss_fn, loss = test_loss(mis, pred, label)
