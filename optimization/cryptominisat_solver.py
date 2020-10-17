import os
import subprocess
from shutil import copyfile

class CryptoMinisatSolver(object):
    """
    https://www.msoos.org/minisat-faq/
    https://www.msoos.org/cryptominisat-faq/
    """

    def __init__(self):
        pass

    def parse_output(self, output, print_info=True):
        output = output.split('\n')
        result = []

        for line in output:
            if line.startswith('v'):
                parts = line.strip().split(' ')[1:]
                result += parts
            elif print_info:
                print(line)

        result = result[:-1]  # remove the last 0
        return ['-' not in x for x in result]

    def solve(self, factors, observed, cnf_file):
        rvs = list(sorted(factors.keys()))
        rv2idx = {rv: i for i, rv in enumerate(rvs)}

        modified_cnf = '/tmp/data.cnf'

        for f in (modified_cnf, ):
            if os.path.exists(f):
                os.remove(f)

        copyfile(cnf_file, modified_cnf)
        with open(modified_cnf, 'a+') as f:
            for rv, rv_val in observed.items():
                s = '{}{} 0\n'.format(
                    '' if rv_val else '-', rv2idx[rv] + 1)
                f.write(s)

        print('Solving...')
        cmd = ['cryptominisat5', modified_cnf]
        output = subprocess.run(cmd,
            stdout=subprocess.PIPE).stdout.decode('utf-8')
        truth_values = self.parse_output(output)
        solution = {rv: truth_values[i] for i, rv in enumerate(rvs)}
        return solution
