# -*- coding: utf-8 -*-
#!/usr/bin/python3

import subprocess

if __name__ == '__main__':
    test_cases = [
        'config/hash_and_input.yaml',
        'config/map_from_input.yaml',
        'config/pseudo_hash.yaml'
    ]

    failed = []

    print('****** RUNNING LBP HASH REVERSAL TESTS ******')
    for test_case in test_cases:
        print(('*' * 30) + '\nTest case: %s' % test_case)
        return_code = subprocess.call('./main %s' % test_case, shell=True)
        status = 'PASS' if return_code == 0 else 'FAIL'
        print('Status: %s' % status)
        if status == 'FAIL':
            failed.append(test_case)

    print('All tests finished.')
    if failed:
        print('The following tests failed:\n\t' + '\n\t'.join(failed))
