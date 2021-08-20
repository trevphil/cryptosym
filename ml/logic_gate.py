import torch
from enum import Enum, unique


@unique
class GateType(Enum):
    and_gate = 'A'
    xor_gate = 'X'
    or_gate = 'O'
    maj_gate = 'M'
    xor3_gate = 'Z'


def inputs_for_gate(gate_type):
    return {
        GateType.and_gate: 2,
        GateType.xor_gate: 2,
        GateType.or_gate: 2,
        GateType.maj_gate: 3,
        GateType.xor3_gate: 3
    }[gate_type]


def gate_type_to_str(gate_type):
    return {
        GateType.and_gate: 'AND',
        GateType.xor_gate: 'XOR2',
        GateType.or_gate: 'OR',
        GateType.maj_gate: 'MAJ3',
        GateType.xor3_gate: 'XOR3'
    }[gate_type]


def needs_gradient_friendly(input_values):
    if type(input_values) not in (list, tuple):
        raise RuntimeError('Input must be a list or tuple!')
    return any(
        (isinstance(x, torch.Tensor) and x.requires_grad)
        for x in input_values
    )


class LogicGate(object):
    def __init__(self, gate_type, output, inputs):
        assert isinstance(output, int)
        assert (isinstance(inputs, list) or isinstance(inputs, tuple))
        assert 0 not in ([output] + list(inputs))
        assert output > 0, 'Gate output should be positive! output = %d' % out
        self.t = GateType(gate_type)
        assert len(inputs) == inputs_for_gate(self.t), 'Wrong # gate inputs!'
        self.output = output
        self.inputs = inputs

    def __hash__(self):
        return hash(f'{self.t.value} {self.output} {self.inputs}')

    def compute_output(self, input_values):
        if needs_gradient_friendly(input_values):
            return self.compute_output_gradient_friendly(input_values)

        assert len(input_values) == inputs_for_gate(self.t)
        if self.t == GateType.and_gate:
            return input_values[0] & input_values[1]
        elif self.t == GateType.xor_gate:
            return input_values[0] ^ input_values[1]
        elif self.t == GateType.or_gate:
            return input_values[0] | input_values[1]
        elif self.t == GateType.maj_gate:
            return (1 if sum(input_values) > 1 else 0)
        elif self.t == GateType.xor3_gate:
            return input_values[0] ^ input_values[1] ^ input_values[2]
        else:
            raise NotImplementedError

    def compute_output_gradient_friendly(self, input_values):
        assert len(input_values) == inputs_for_gate(self.t)

        if self.t == GateType.and_gate:
            return input_values[0] * input_values[1]
        elif self.t == GateType.xor_gate:
            a, b = input_values
            tmp1 = 1 - (a * b)
            tmp2 = 1 - ((1 - a) * (1 - b))
            return tmp1 * tmp2
        elif self.t == GateType.or_gate:
            a, b = input_values
            return 1 - ((1 - a) * (1 - b))
        elif self.t == GateType.maj_gate:
            a, b, c = input_values
            tmp1 = 1 - ((1 - a) * (1 - b))
            tmp2 = 1 - ((1 - a) * (1 - c))
            tmp3 = 1 - ((1 - b) * (1 - c))
            return tmp1 * tmp2 * tmp3
        elif self.t == GateType.xor3_gate:
            a, b, c = input_values
            tmp1 = 1 - (a * b * (1 - c))
            tmp2 = 1 - (a * (1 - b) * c)
            tmp3 = 1 - ((1 - a) * b * c)
            tmp4 = 1 - ((1 - a) * (1 - b) * (1 - c))
            return tmp1 * tmp2 * tmp3 * tmp4
        else:
            raise NotImplementedError

    def cnf_clauses(self):
        if self.t == GateType.and_gate:
            return [
                [-self.output, self.inputs[0]],
                [-self.output, self.inputs[1]],
                [self.output, -self.inputs[0], -self.inputs[1]]
            ]
        elif self.t == GateType.xor_gate:
            return [
                [self.output, self.inputs[0], -self.inputs[1]],
                [self.output, -self.inputs[0], self.inputs[1]],
                [-self.output, self.inputs[0], self.inputs[1]],
                [-self.output, -self.inputs[0], -self.inputs[1]]
            ]
        elif self.t == GateType.or_gate:
            return [
                [self.output, -self.inputs[0]],
                [self.output, -self.inputs[1]],
                [-self.output, self.inputs[0], self.inputs[1]]
            ]
        elif self.t == GateType.maj_gate:
            return [
                [-self.output, self.inputs[0], self.inputs[1]],
                [-self.output, self.inputs[0], self.inputs[2]],
                [-self.output, self.inputs[1], self.inputs[2]],
                [self.output, -self.inputs[0], -self.inputs[1]],
                [self.output, -self.inputs[0], -self.inputs[2]],
                [self.output, -self.inputs[1], -self.inputs[2]]
            ]
        elif self.t == GateType.xor3_gate:
            return [
                [self.output, self.inputs[0], self.inputs[1], -self.inputs[2]],
                [self.output, self.inputs[0], -self.inputs[1], self.inputs[2]],
                [self.output, -self.inputs[0], self.inputs[1], self.inputs[2]],
                [self.output, -self.inputs[0], -self.inputs[1], -self.inputs[2]],
                [-self.output, self.inputs[0], self.inputs[1], self.inputs[2]],
                [-self.output, self.inputs[0], -self.inputs[1], -self.inputs[2]],
                [-self.output, -self.inputs[0], self.inputs[1], -self.inputs[2]],
                [-self.output, -self.inputs[0], -self.inputs[1], self.inputs[2]]
            ]
        else:
            raise NotImplementedError        


if __name__ == '__main__':
    t = torch.zeros(10, requires_grad=True)
    assert needs_gradient_friendly([t])
    assert needs_gradient_friendly((t, ))
    t = torch.zeros(10, requires_grad=False)
    assert not needs_gradient_friendly([t])
    assert not needs_gradient_friendly((t, ))
    assert not needs_gradient_friendly([1, 0, 1])

    gate_computation = {
        GateType.and_gate: (lambda x: x[0] & x[1]),
        GateType.xor_gate: (lambda x: x[0] ^ x[1]),
        GateType.or_gate: (lambda x: x[0] | x[1]),
        GateType.maj_gate: (lambda x: (1 if sum(x) > 1 else 0)),
        GateType.xor3_gate: (lambda x: x[0] ^ x[1] ^ x[2])
    }

    for gate_type in GateType:
        num_inputs = inputs_for_gate(gate_type)
        inputs = list(range(1, num_inputs + 1))
        output = num_inputs + 1
        gate = LogicGate(gate_type, output=output, inputs=inputs)
        print(f'\nTesting gate: {gate_type.value}')
        for i in range(1 << num_inputs):
            input_values = [(i >> x) & 1 for x in range(num_inputs)]
            expected = gate_computation[gate_type](input_values)
            v1 = gate.compute_output(input_values)
            v2 = gate.compute_output_gradient_friendly(input_values)
            binstr = format(i, f'0{num_inputs}b')
            print(f'Testing input: {binstr} --> {(expected, v1, v2)}')
            assert expected == v1
            assert expected == v2
