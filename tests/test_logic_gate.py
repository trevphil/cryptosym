import pytest

from cryptosym import LogicGate

GType = LogicGate.Type


def is_sat(clauses: list[list[int]], values: dict[int, bool]) -> bool:
    for clause in clauses:
        is_sat = False
        for lit in clause:
            if values[lit] if lit > 0 else not values[-lit]:
                is_sat = True
                break
        if not is_sat:
            return False

    return True


class TestLogicGate:
    def test_initialization(self):
        g = LogicGate(t=GType.and_gate, output=3, inputs=[1, 2])
        assert g.t == GType.and_gate
        assert g.output == 3
        assert g.inputs[0] == 1
        assert g.inputs[1] == 2

    def test_string_conversion(self):
        s = "M 4 -1 2 -3"
        g = LogicGate(string=s)
        assert g.t == GType.maj3_gate
        assert g.gate_type == "Maj-3"
        assert g.output == 4
        assert g.inputs[0] == -1
        assert g.inputs[1] == 2
        assert g.inputs[2] == -3
        assert str(g) == s

    def test_set_readonly_properties(self):
        g = LogicGate("M 4 -1 2 -3")
        with pytest.raises(AttributeError):
            g.t = GType.and_gate
        with pytest.raises(AttributeError):
            g.output = 5
        with pytest.raises(AttributeError):
            g.inputs = [1, 2, 3]
        with pytest.raises(AttributeError):
            g.gate_type = "foobar"

    def test_and_gate_cnf(self):
        g = LogicGate(GType.and_gate, 3, [1, 2])
        clauses = g.cnf()
        for i in range(1 << 3):
            b1 = (i >> 0) & 1
            b2 = (i >> 1) & 1
            b3 = (i >> 2) & 1
            expected = b1 & b2
            if b3 == expected:
                assert is_sat(clauses, {1: b1, 2: b2, 3: b3})
            else:
                assert not is_sat(clauses, {1: b1, 2: b2, 3: b3})

    def test_or_gate_cnf(self):
        g = LogicGate(GType.or_gate, 3, [1, 2])
        clauses = g.cnf()
        for i in range(1 << 3):
            b1 = (i >> 0) & 1
            b2 = (i >> 1) & 1
            b3 = (i >> 2) & 1
            expected = b1 | b2
            if b3 == expected:
                assert is_sat(clauses, {1: b1, 2: b2, 3: b3})
            else:
                assert not is_sat(clauses, {1: b1, 2: b2, 3: b3})

    def test_xor_gate_cnf(self):
        g = LogicGate(GType.xor_gate, 3, [1, 2])
        clauses = g.cnf()
        for i in range(1 << 3):
            b1 = (i >> 0) & 1
            b2 = (i >> 1) & 1
            b3 = (i >> 2) & 1
            expected = b1 ^ b2
            if b3 == expected:
                assert is_sat(clauses, {1: b1, 2: b2, 3: b3})
            else:
                assert not is_sat(clauses, {1: b1, 2: b2, 3: b3})

    def test_xor3_gate_cnf(self):
        g = LogicGate(GType.xor3_gate, 4, [1, 2, 3])
        clauses = g.cnf()
        for i in range(1 << 4):
            b1 = (i >> 0) & 1
            b2 = (i >> 1) & 1
            b3 = (i >> 2) & 1
            b4 = (i >> 3) & 1
            expected = b1 ^ b2 ^ b3
            if b4 == expected:
                assert is_sat(clauses, {1: b1, 2: b2, 3: b3, 4: b4})
            else:
                assert not is_sat(clauses, {1: b1, 2: b2, 3: b3, 4: b4})

    def test_maj3_gate_cnf(self):
        g = LogicGate(GType.maj3_gate, 4, [1, 2, 3])
        clauses = g.cnf()
        for i in range(1 << 4):
            b1 = (i >> 0) & 1
            b2 = (i >> 1) & 1
            b3 = (i >> 2) & 1
            b4 = (i >> 3) & 1
            expected = (int(b1) + int(b2) + int(b3)) > 1
            if b4 == expected:
                assert is_sat(clauses, {1: b1, 2: b2, 3: b3, 4: b4})
            else:
                assert not is_sat(clauses, {1: b1, 2: b2, 3: b3, 4: b4})

    def test_wrong_number_of_inputs(self):
        with pytest.raises(ValueError):
            _ = LogicGate(GType.and_gate, 2, [1])
        with pytest.raises(ValueError):
            _ = LogicGate(GType.and_gate, 4, [1, 2, 3])

        with pytest.raises(ValueError):
            _ = LogicGate(GType.or_gate, 2, [1])
        with pytest.raises(ValueError):
            _ = LogicGate(GType.or_gate, 4, [1, 2, 3])

        with pytest.raises(ValueError):
            _ = LogicGate(GType.xor_gate, 2, [1])
        with pytest.raises(ValueError):
            _ = LogicGate(GType.xor_gate, 4, [1, 2, 3])

        with pytest.raises(ValueError):
            _ = LogicGate(GType.xor3_gate, 3, [1, 2])
        with pytest.raises(ValueError):
            _ = LogicGate(GType.and_gate, 5, [1, 2, 3, 4])

        with pytest.raises(ValueError):
            _ = LogicGate(GType.maj3_gate, 3, [1, 2])
        with pytest.raises(ValueError):
            _ = LogicGate(GType.maj3_gate, 5, [1, 2, 3, 4])

    def test_negated_output(self):
        with pytest.raises(ValueError):
            _ = LogicGate(GType.xor_gate, -3, [1, 2])

    def test_zero_indexed_variables(self):
        with pytest.raises(ValueError):
            _ = LogicGate(GType.or_gate, 0, [1, 2])
        with pytest.raises(ValueError):
            _ = LogicGate(GType.or_gate, 2, [0, 1])
