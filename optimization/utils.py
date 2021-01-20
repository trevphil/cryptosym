# -*- coding: utf-8 -*-

import scipy.io


def save_to_matlab_matrix(matrices, filename, verbose=True):
    """
    Save a numpy matrix to MATLAB format
    """
    scipy.io.savemat(filename, matrices)
    if verbose:
        print('Saved data to %s' % filename)


def set_implicit_observed(factors, observed, all_bits, verbose=True):
    """
    Some bits can be figured out, e.g. if bit X = INV(bit Y) and
    bit X is observed as 0, then we can infer that bit Y = 1.
    If bit X = 1 = AND(bit Y, bit Z) then bits Y and Z are both 1.
    """

    initial = len(observed)
    if verbose:
        print('There are %d initially observed bits.' % initial)

    def backward(observed):
        queue = list(observed.keys())
        smallest_obs = len(all_bits)

        while len(queue) > 0:
            rv = queue.pop(0)
            smallest_obs = min(smallest_obs, rv)
            observed[rv] = bool(all_bits[rv])
            factor = factors[rv]
            if factor.factor_type in ('INV', 'SAME'):
                inp = factor.input_rvs[0]
                queue.append(inp)
            elif factor.factor_type == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                if observed[rv] == True:
                    # Output is observed to be 1, so inputs must be 1
                    queue.append(inp1)
                    queue.append(inp2)
                elif inp1 in observed.keys() and observed[inp1] == True:
                    # Output is 0, inp1 is 1, so inp2 must be 0
                    queue.append(inp2)
                elif inp2 in observed.keys() and observed[inp2] == True:
                    # Output is 0, inp2 is 1, so inp1 must be 0
                    queue.append(inp1)

        return observed, smallest_obs

    def forward(observed, smallest_obs):
        obs_rv_set = set(observed.keys())
        for rv in range(smallest_obs, len(all_bits)):
            factor = factors.get(rv, None)
            if factor is None:
                continue
            if factor.factor_type in ('INV', 'SAME'):
                inp = factor.input_rvs[0]
                if inp in obs_rv_set:
                    observed[rv] = bool(all_bits[rv])
            elif factor.factor_type == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                inp1_is_observed = (inp1 in obs_rv_set)
                inp2_is_observed = (inp2 in obs_rv_set)
                if inp1_is_observed and inp2_is_observed:
                    observed[rv] = bool(all_bits[rv])
                elif inp1_is_observed and observed[inp1] == False:
                    observed[rv] = bool(all_bits[rv])
                elif inp2_is_observed and observed[inp2] == False:
                    observed[rv] = bool(all_bits[rv])
        return observed

    while True:
        before = len(observed)
        observed, smallest_obs = backward(observed)
        observed = forward(observed, smallest_obs)
        after = len(observed)
        if before == after:
            break  # No new observed variables were derived

    if verbose:
        print('Found %d additional observed bits.' % (len(observed) - initial))
    return observed
