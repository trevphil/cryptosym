# -*- coding: utf-8 -*-

import os
import yaml
import numpy as np
from matplotlib import pyplot as plt
from collections import defaultdict

from optimization.main import main, load_factors


def log_ticks(x):
    x_min = int(np.round(np.min(x)) - 1)
    x_max = int(np.round(np.max(x)) + 1)
    ticks = range(x_min, x_max)
    labels = [r'$10^{%d}$' % i for i in ticks]
    return ticks, labels


def plot_size_vs_runtime(stats, max_size, save_to=None):
    if type(stats) is str:
        with open(stats, 'r') as f:
            stats = yaml.safe_load(f)

    x = np.log10(stats['problem_size'])
    y = np.log10(stats['runtime'])
    m, b = np.polyfit(x, y, 1)  # y = mx + b
    x_fit = x.copy()
    y_fit = m * x_fit + b

    # Predict the solve time for the maximum problem size
    x = np.hstack((x, np.log10(max_size)))
    y_pred = m * np.log10(max_size) + b
    y = np.hstack((y, y_pred))
    print('Problem size %d is predicted to take %.0f minutes' % (
        max_size, np.round(np.power(10, y_pred) / 60)))

    ax = plt.axes()
    ax.set_title('%s: problem size vs. solve time' % stats['solver'])
    colors = [(0, 0, 1) for _ in range(x.shape[0] - 1)] + [(1, 0, 0)]
    ax.scatter(x, y, c=colors)
    ax.plot(x_fit, y_fit)
    x_ticks, x_labels = log_ticks(x)
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_labels)
    y_ticks, y_labels = log_ticks(y)
    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel('problem size')
    ax.set_ylabel('solve time [s]')

    if save_to is not None:
        plt.savefig(save_to)
        print('Saved figure to: %s' % save_to)
    plt.show()


def plot_factors_vs_difficulty(datasets, difficulties, save_to=None):
    num_prior, num_and, num_inv, num_same = [], [], [], []

    for i, dataset in enumerate(datasets):
        factor_file = os.path.join(dataset, 'factors.txt')
        factors = load_factors(factor_file)
        factor_counts = defaultdict(lambda: 0)
        for f in factors.values():
            factor_counts[f.factor_type] += 1
        num_prior.append(factor_counts['PRIOR'])
        num_same.append(factor_counts['SAME'])
        num_and.append(factor_counts['AND'])
        num_inv.append(factor_counts['INV'])

    ax = plt.axes()
    ax.set_title('SHA-256: # factors vs. # rounds')
    ax.plot(difficulties, num_prior, label='PRIOR')
    ax.plot(difficulties, num_same, label='SAME')
    ax.plot(difficulties, num_and, label='AND')
    ax.plot(difficulties, num_inv, label='INV')
    ax.legend()
    ax.axvline(17)
    ax.set_xticks(np.linspace(1, 64, 8))

    if save_to is not None:
        plt.savefig(save_to)
        print('Saved figure to: %s' % save_to)
    plt.show()


def eval_solver(solver, max_size):
    difficulties = [1, 4, 8, 12, 16, 17, 18, 19, 20, 21, 22, 23, 24, 32, 64]
    datasets = ['data/sha256_d%d' % d for d in difficulties]
    plot_factors_vs_difficulty(datasets, difficulties,
        save_to='images/sha256_factors.pdf')

    problem_size = []
    difficulty = []
    runtime = []
    success = []

    for i, dataset in enumerate(datasets):
        print('\nTrying difficulty %d...\n' % difficulties[i])
        try:
            stats = main(dataset, solver)
        except:
            break
        problem_size += stats['problem_size']
        difficulty += stats['difficulty']
        runtime += stats['runtime']
        success += stats['success']

    stats = {
        'solver': solver,
        'problem_size': problem_size,
        'difficulty': difficulty,
        'runtime': runtime,
        'success': success
    }

    stats_file = 'stats.yaml'
    with open(stats_file, 'w') as f:
        yaml.dump(stats, f, default_flow_style=None)
        print('Saved stats to: %s' % stats_file)

    plot_size_vs_runtime(stats, max_size=max_size,
        save_to='images/%s_size_vs_time.pdf' % solver)


if __name__ == '__main__':
    MAX_SIZE = 500791  # for SHA-256 at full difficulty
    eval_solver('sat', MAX_SIZE)
    # plot_size_vs_runtime('stats.yaml',
    #     max_size=MAX_SIZE, save_to='images/sat_size_vs_time.pdf')
