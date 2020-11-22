import os
import subprocess
from shutil import copyfile

class MinisatSolver(object):
    """
    https://www.msoos.org/minisat-faq/
    https://www.msoos.org/cryptominisat-faq/
    """

    def __init__(self):
        pass

    def parse_output(self, outfile):
        with open(outfile, 'r') as f:
            line = f.readlines()[1].strip()
        parts = line.split(' ')[:-1]
        return ['-' not in rv for rv in parts]

    def solve(self, factors, observed, cnf_file):
        rvs = list(sorted(factors.keys()))
        rv2idx = {rv: i for i, rv in enumerate(rvs)}

        modified_cnf = '/tmp/data.cnf'
        simplified_cnf = '/tmp/simplified.cnf'
        output_file = '/tmp/out.txt'

        for f in (modified_cnf, simplified_cnf, output_file):
            if os.path.exists(f):
                os.remove(f)

        copyfile(cnf_file, modified_cnf)
        with open(modified_cnf, 'a+') as f:
            for rv, rv_val in observed.items():
                s = '{}{} 0\n'.format(
                    '' if rv_val else '-', rv2idx[rv] + 1)
                f.write(s)

        # simplify
        cmd = ['minisat', '-no-solve',
            '-dimacs=%s' % simplified_cnf, modified_cnf]
        output = subprocess.run(cmd,
            stdout=subprocess.PIPE).stdout.decode('utf-8')
        print(output)

        cmd = ['minisat', simplified_cnf, output_file]
        output = subprocess.run(cmd,
            stdout=subprocess.PIPE).stdout.decode('utf-8')
        print(output)
        truth_values = self.parse_output(output_file)
        solution = {rv: truth_values[i] for i, rv in enumerate(rvs)}
        return solution
