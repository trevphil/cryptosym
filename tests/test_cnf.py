"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

from pathlib import Path

import pytest

from cryptosym import CNF


class TestCNF:
    def test_empty(self):
        cnf = CNF()
        assert cnf.num_vars == 0
        assert cnf.num_clauses == 0
        assert len(cnf.clauses) == 0

    def test_set_readonly_properties(self):
        cnf = CNF()
        with pytest.raises(AttributeError):
            cnf.num_vars = 1
        with pytest.raises(AttributeError):
            cnf.num_clauses = 1
        with pytest.raises(AttributeError):
            cnf.clauses = [[1, 2]]

    def test_initialization(self):
        cnf = CNF(clauses=[{1, 2, -3}, {-2, 4}], num_vars=4)
        assert cnf.num_vars == 4
        assert cnf.num_clauses == 2
        assert cnf.clauses[0] == {1, 2, -3}
        assert cnf.clauses[1] == {-2, 4}

    def test_simplification(self):
        cnf = CNF([{1, 2, -3}, {-2, 4}], 4)

        cnf = cnf.simplify(assignments={2: False})
        assert cnf.num_vars == 2
        assert cnf.num_clauses == 1
        assert cnf.clauses[0] == {-1, 2}

        cnf = cnf.simplify({1: False})
        assert cnf.num_clauses == 0
        assert cnf.num_vars == 0
        assert len(cnf.clauses) == 0

    def test_approximation_ratio(self):
        cnf = CNF([{-1, 2}, {3, -4}, {-5, 6}, {7, -8}], 8)
        assignments = {
            1: True,
            2: False,
            3: False,
            4: True,
            5: True,
            6: False,
            7: False,
            8: False,
        }
        assert cnf.num_sat_clauses(assignments=assignments) == 1
        assert cnf.approximation_ratio(assignments=assignments) == pytest.approx(0.25)

    def test_num_sat_clauses_partial_assignment(self):
        cnf = CNF([{-1, 2}, {3, -4}], 4)
        assignments = {-1: True, 3: False}
        with pytest.raises(IndexError):
            _ = cnf.num_sat_clauses(assignments)

    def test_read_write(self, tmp_path: Path):
        cnf = CNF([{-1, 2}, {3, -4}], 8)
        cnf.to_file(file_path=tmp_path / "foo.cnf")
        loaded = CNF(file_path=tmp_path / "foo.cnf")
        assert loaded.num_vars == 8
        assert loaded.num_clauses == 2
        assert loaded.clauses[0] == {-1, 2}
        assert loaded.clauses[1] == {-4, 3}

    def test_trim_spaces(self, tmp_path: Path):
        with open(tmp_path / "whitespace.cnf", "w") as f:
            f.write("# This is a comment \n")
            f.write(" # Also a comment\n")
            f.write("p cnf 3 2 \r\n")
            f.write("# Another comment\n")
            f.write(" 2 -1   3\t0 \r\n")
            f.write("\t-3 2 0\n")
            f.write("\n")
        cnf = CNF(tmp_path / "whitespace.cnf")
        assert cnf.num_vars == 3
        assert cnf.num_clauses == 2
        assert cnf.clauses[0] == {-1, 2, 3}
        assert cnf.clauses[1] == {-3, 2}

    def test_load_from_nonexistant_file(self):
        with pytest.raises(ValueError):
            _ = CNF("does_not_exist.cnf")

    def test_load_dimacs_without_header(self, tmp_path: Path):
        with open(tmp_path / "no_header.cnf", "w") as f:
            f.write("1 2 3 0\n")
            f.write("3 -1 -4 0\n")
        with pytest.raises(RuntimeError):
            _ = CNF(str(tmp_path / "no_header.cnf"))

    def test_simplify_with_zero_indexed_assignments(self):
        cnf = CNF([{-1, 2}, {3, -4}], num_vars=8)
        with pytest.raises(ValueError):
            _ = cnf.simplify({2: True, 0: False})

    def test_simplify_results_in_unsatisfiability(self):
        cnf = CNF([{-1, 2}, {-2, 3}], num_vars=3)
        with pytest.raises(RuntimeError):
            _ = cnf.simplify({-1: False, 3: False})
