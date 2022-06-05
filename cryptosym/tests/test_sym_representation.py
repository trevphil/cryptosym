"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

from pathlib import Path

import pytest

from cryptosym import LogicGate, SymRepresentation

GType = LogicGate.Type


class TestSymRepresentation:
    def test_initialization(self):
        gates = [
            LogicGate(GType.and_gate, 4, [1, -2]),
            LogicGate(GType.xor_gate, 5, [2, -3]),
        ]
        rep = SymRepresentation(
            gates=gates, input_indices=[1, 2, 3], output_indices=[0, 4, 5]
        )
        assert rep.num_vars == 5
        assert len(rep.gates) == 2
        assert rep.input_indices == [1, 2, 3]
        assert rep.output_indices == [0, 4, 5]

    def test_set_readonly_properties(self):
        gates = [
            LogicGate(GType.and_gate, 4, [1, -2]),
            LogicGate(GType.xor_gate, 5, [2, -3]),
        ]
        rep = SymRepresentation(gates, [1, 2, 3], [0, 4, 5])
        with pytest.raises(AttributeError):
            rep.num_vars = 10
        with pytest.raises(AttributeError):
            rep.input_indices = [10, 20, 30]
        with pytest.raises(AttributeError):
            rep.output_indices = [10, 20, 30]
        with pytest.raises(AttributeError):
            rep.gates = []

    def test_prune_and_reindex(self):
        gates = [
            LogicGate(GType.and_gate, 4, [1, -2]),
            LogicGate(GType.and_gate, 5, [3, -4]),
        ]
        rep = SymRepresentation(gates, [1, 2, 3], [4])
        assert rep.num_vars == 3
        assert len(rep.gates) == 1
        assert rep.gates[0].t == GType.and_gate
        assert rep.input_indices == [1, 2, 0]
        assert rep.output_indices == [3]

    def test_convert_to_cnf(self):
        gates = [
            LogicGate(GType.and_gate, 4, [1, -2]),
            LogicGate(GType.and_gate, 5, [3, -4]),
        ]
        rep = SymRepresentation(gates, [1, 2, 3], [4])
        cnf = rep.to_cnf()
        assert cnf.num_vars == 3
        assert cnf.num_clauses == 3

    def test_convert_dag(self, tmp_path: Path):
        gates = [
            LogicGate(GType.and_gate, 4, [1, -2]),
            LogicGate(GType.xor_gate, 5, [2, -3]),
            LogicGate(GType.maj3_gate, 6, [1, 4, 5]),
        ]
        inputs = [1, 2, 3]
        outputs = [0, 6, 5, 0, 0]
        rep = SymRepresentation(gates, inputs, outputs)
        assert rep.num_vars == 6
        assert len(rep.gates) == 3
        assert rep.input_indices == inputs
        assert rep.output_indices == outputs

        rep.to_dag(tmp_path / "dag.txt")
        loaded = SymRepresentation(dag_file=tmp_path / "dag.txt")
        assert loaded.num_vars == 6
        assert len(loaded.gates) == 3
        assert loaded.input_indices == inputs
        assert loaded.output_indices == outputs

    def test_load_invalid_dag(self):
        with pytest.raises(ValueError):
            _ = SymRepresentation("does_not_exist.txt")
        with pytest.raises(ValueError):
            _ = SymRepresentation(Path("does_not_exist.txt"))
