import random
import shutil
import subprocess
from pathlib import Path
from math import comb
from typing import Union
from threading import Timer
from sympy import divisors

import pysat
from pysat.solvers import Glucose4
from pysat.formula import CNF as PyCNF

tmp = Path("/tmp/pysat")
pysat.params["data_dirs"] = str(tmp)


def randi(lb: Union[int, float], ub: Union[int, float]) -> int:
    if isinstance(lb, float):
        lb = int(lb + 0.5)
    if isinstance(ub, float):
        ub = int(ub + 0.5)
    if lb == ub:
        return lb
    return random.randint(lb, ub)


def round_up(x: int, divisor: int) -> int:
    if x % divisor == 0:
        return x
    return x + divisor - (x % divisor)


def execute_cnfgen(cmd: str) -> str:
    # https://massimolauria.net/cnfgen/transformation.html
    cmd += " -T shuffle"
    proc = subprocess.Popen(cmd.split(" "), stdout=subprocess.PIPE)
    return proc.stdout.read().decode("UTF-8")


def attempt_solve(cnf: str, timeout: float = 5) -> Union[None, bool]:
    # https://pysathq.github.io/docs/html/api/solvers.html
    def _interrupt(s):
        s.interrupt()

    solver = Glucose4(bootstrap_with=PyCNF(from_string=cnf).clauses)
    timer = Timer(timeout, _interrupt, [solver])
    timer.start()

    status = solver.solve_limited(expect_interrupt=True)
    solver.delete()
    return status


def pigeonhole_sat() -> str:
    n = randi(3, 25)
    m = randi(2, n)
    assert m <= n
    return execute_cnfgen(f"cnfgen php {m} {n}")


def pigeonhole_unsat() -> str:
    n = randi(3, 20)
    m = randi(n + 1, n * 1.5)
    assert m > n
    return execute_cnfgen(f"cnfgen php {m} {n}")


def tseitin_unsat() -> str:
    n = randi(5, 20)
    assert n > 4
    return execute_cnfgen(f"cnfgen tseitin {n}")


def subset_cardinality_unsat() -> str:
    n = randi(5, 17)
    assert n > 4
    return execute_cnfgen(f"cnfgen subsetcard {n}")


def parity_sat() -> str:
    n = round_up(randi(4, 20), 2)
    assert n % 2 == 0
    return execute_cnfgen(f"cnfgen parity {n}")


def parity_unsat() -> str:
    n = round_up(randi(4, 20), 2) - 1
    assert n % 2 == 1
    return execute_cnfgen(f"cnfgen parity {n}")


def counting_principle_sat() -> str:
    m, d = -1, -1
    while d == -1:
        m = randi(6, 10)
        d_choices = divisors(m)
        if len(d_choices) > 2:
            d = random.choice(d_choices[1:-1])
    assert d < m and m % d == 0
    return execute_cnfgen(f"cnfgen count {m} {d}")


def counting_principle_unsat() -> str:
    m, d = -1, -1
    while d == -1:
        m = randi(6, 12)
        d_choices = set(range(1, m)) - set(divisors(m))
        if len(d_choices) > 0:
            d = random.choice(list(d_choices))
    assert d < m and m % d != 0
    return execute_cnfgen(f"cnfgen count {m} {d}")


def pebbling_formula_unsat() -> str:
    h = randi(2, 10)
    assert h > 1
    method = random.choice(["pyramid", "tree", "path"])
    return execute_cnfgen(f"cnfgen peb {method} {h} -T xor 2")


def clique_coloring_unsat() -> str:
    n = randi(5, 15)
    assert n > 4
    k = int((n ** (2.0 / 3.0)) + 0.5)
    c = k - 1
    return execute_cnfgen(f"cnfgen cliquecoloring {n} {k} {c}")


def ramsey_unsat() -> str:
    r = randi(3, 6)
    s = randi(2, r - 1)
    assert s < r
    max_ramsey = comb(r + s - 2, r - 1)
    # It is TRUE that ramsey(r, s) <= max_ramsey
    # Build (wrong) proof that ramsey(r, s) > n  -->  UNSAT
    n = randi(max_ramsey, max_ramsey + 4)
    return execute_cnfgen(f"cnfgen ram {r} {s} {n}")


def rand3_sat() -> str:
    while True:
        n = randi(10, 200)
        c = randi(n, n * 4)
        cnf = execute_cnfgen(f"cnfgen randkcnf 3 {n} {c}")
        status = attempt_solve(cnf)
        if status is True:
            return cnf


def rand3_unsat() -> str:
    while True:
        n = randi(10, 200)
        c = randi(n * 4.5, n * 6)
        cnf = execute_cnfgen(f"cnfgen randkcnf 3 {n} {c}")
        status = attempt_solve(cnf)
        if status is False:
            return cnf


MAX_CLAUSES = 50000


ALL_PROBLEMS = [
    ("unsat", pebbling_formula_unsat),
    ("sat", counting_principle_sat),
    ("unsat", counting_principle_unsat),
    ("sat", rand3_sat),
    ("unsat", rand3_unsat),
    ("sat", pigeonhole_sat),
    ("unsat", pigeonhole_unsat),
    ("unsat", tseitin_unsat),
    ("unsat", subset_cardinality_unsat),
    ("sat", parity_sat),
    ("unsat", parity_unsat),
    ("unsat", clique_coloring_unsat),
]


DATASET = {
    "train": [
        ("sat", "rand3", rand3_sat, 9000),
        ("unsat", "rand3", rand3_unsat, 5000),
        ("sat", "parity", parity_sat, 1000),
        ("unsat", "parity", parity_unsat, 1000),
        ("unsat", "pebbling-formula", pebbling_formula_unsat, 1000),
        ("unsat", "clique-coloring", clique_coloring_unsat, 1000),
        ("unsat", "tseitin", tseitin_unsat, 1000),
        ("unsat", "subset-cardinality", subset_cardinality_unsat, 1000),
    ],
    "val": [
        ("sat", "counting-principle", counting_principle_sat, 500),
        ("unsat", "counting-principle", counting_principle_unsat, 500),
    ],
    "test": [
        ("sat", "pigeonhole", pigeonhole_sat, 500),
        ("unsat", "pigeonhole", pigeonhole_unsat, 500),
    ],
}


def test_satisfiability() -> None:
    random.seed(2)

    def _get_nv_nc(cnf_str):
        for line in cnf_str.splitlines():
            if line.startswith("p"):
                return line
        return None

    for is_sat, func in ALL_PROBLEMS:
        print(f"Testing: {func.__name__}")

        validated = 0
        while validated < 5:
            cnf = func()
            print(f"\t{_get_nv_nc(cnf)}")
            status = attempt_solve(cnf, timeout=2.5)
            if status is True and is_sat == "unsat":
                assert False, "Problem is SAT! Expected UNSAT."
            elif status is False and is_sat == "sat":
                assert False, "Problem is UNSAT! Expected SAT."
            validated += 1 if isinstance(status, bool) else 0

        print("#" * 50)


if __name__ == "__main__":
    # test_satisfiability()

    out_path = Path("./cnf_files")

    if out_path.exists():
        answer = input(f"'{str(out_path)}' exists. Overwrite? [Y/n] ")
        if answer != "Y":
            exit()
        shutil.rmtree(out_path)

    for split_name, info in DATASET.items():
        print(f"Starting split: {split_name}")
        (out_path / split_name).mkdir(parents=True, exist_ok=False)
        num_sat, num_unsat = 0, 0
        overall_sample_idx = 0
        for sat, problem_type, problem_generator, num_problems in info:
            print(f"\tCreating {num_problems} of type: {problem_type} ({sat})")
            problem_idx = 0
            while problem_idx < num_problems:
                cnf_str = problem_generator()

                cnf = PyCNF(from_string=cnf_str)
                if len(cnf.clauses) > MAX_CLAUSES:
                    continue

                fname = f"{problem_type}_{sat}_{overall_sample_idx:05d}.cnf"
                fpath = out_path / split_name / fname
                with open(fpath, "w") as f:
                    f.write(cnf_str)

                problem_idx += 1
                overall_sample_idx += 1
                if sat == "sat":
                    num_sat += 1
                else:
                    num_unsat += 1

                if problem_idx % 100 == 0:
                    print(f"\t Completed {problem_idx}/{num_problems}")

        print(f"\tN={overall_sample_idx}")
        print(f"\tSAT={num_sat}")
        print(f"\tUNSAT={num_unsat}")
