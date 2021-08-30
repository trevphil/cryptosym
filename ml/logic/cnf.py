from copy import deepcopy
from collections import defaultdict


class CNF(object):
    def __init__(self, problem):
        self.clauses = []
        self.lit_to_clause_indices = defaultdict(lambda: set())
        self.num_vars = problem.num_vars

        for gate in problem.gates:
            for clause in gate.cnf_clauses():
                self.clauses.append(set(clause))

        self.reindex_lit_to_clause()

    @property
    def num_clauses(self):
        return len(self.clauses)

    def __repr__(self):
        s = ''
        for i, clause in enumerate(self.clauses):
            s += f'{list(sorted(clause))}'
            if i != len(self.clauses) - 1:
                s += '\n'
        return s

    def reindex_lit_to_clause(self):
        self.lit_to_clause_indices = defaultdict(lambda: set())
        clause_idx = 0
        new_clauses = []

        for clause in self.clauses:
            if clause is None:
                continue  # Skip

            clause_is_auto_sat = False
            for lit in clause:
                if (-lit) in clause:
                    clause_is_auto_sat = True
                    break

            if clause_is_auto_sat:
                continue  # Skip

            for lit in clause:
                self.lit_to_clause_indices[lit].add(clause_idx)

            new_clauses.append(clause)
            clause_idx += 1

        self.clauses = new_clauses

    def simplify(self, assumptions):
        cnf = deepcopy(self)
        assumptions = deepcopy(assumptions)
        queue = []

        for lit, val in assumptions.items():
            assert lit > 0
            queue.append((lit, val))
            queue.append((-lit, 1 - val))

        while len(queue) > 0:
            lit, val = queue.pop()
            referenced_clauses = cnf.lit_to_clause_indices[lit]
            if val:
                for clause_idx in referenced_clauses:
                    # All referenced clauses are automatically SAT
                    cnf.clauses[clause_idx] = None  # Remove clause
            else:
                for clause_idx in referenced_clauses:
                    clause = cnf.clauses[clause_idx]
                    if clause is None:
                        continue

                    # If this is the last literal in the clause, UNSAT
                    if len(clause) == 1:
                        return None

                    # Remove literal from clause, since it is 0 / false
                    clause.discard(lit)

                    # If clause now has only 1 lit, add it to assumptions
                    if len(clause) == 1:
                        last_lit = next(iter(clause))

                        if last_lit in assumptions and assumptions[last_lit] != 1:
                            return None  # UNSAT because we need last_lit = 1
                        if (-last_lit) in assumptions and assumptions[-last_lit] != 0:
                            return None  # UNSAT, need -last_lit = 0 --> last_lit = 1

                        queue.append((last_lit, 1))
                        queue.append((-last_lit, 0))
                        assumptions[last_lit] = 1
                        assumptions[-last_lit] = 0
                        clause = None  # Remove clause (because now it is SAT)

                    cnf.clauses[clause_idx] = clause

        cnf.reindex_lit_to_clause()
        return cnf

    def is_sat(self, assignments):
        for clause in self.clauses:
            clause_is_sat = any(assignments[lit] for lit in clause)
            if not clause_is_sat:
                return False
        return True
