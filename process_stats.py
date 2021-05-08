import sys
import numpy as np
from matplotlib import pyplot as plt

def main(stats_filename):
  factor_counts = dict()
  and_gap = None
  xor_gap = None

  with open(stats_filename, 'r') as f:
    line = f.readline()
    while line:
      if line.startswith('factors'):
        n_factors = int(line.split()[-1])
        for i in range(n_factors):
          parts = f.readline().strip().split()
          factor_counts[parts[0]] = int(parts[1])
        print(factor_counts)
      elif line.startswith('and_gap_count'):
        n_gaps = int(line.split()[-1])
        and_gap = np.zeros((n_gaps, 2))
        for i in range(n_gaps):
          parts = list(map(int, f.readline().split()))
          and_gap[i, :] = (parts[0], parts[1])
      elif line.startswith('xor_gap_count'):
        n_gaps = int(line.split()[-1])
        xor_gap = np.zeros((n_gaps, 2))
        for i in range(n_gaps):
          parts = list(map(int, f.readline().split()))
          xor_gap[i, :] = (parts[0], parts[1])

      line = f.readline()

  def mean_(val, freq):
    return np.average(val, weights = freq)

  def median_(val, freq):
    order = np.argsort(val)
    cdf = np.cumsum(freq[order])
    return val[order][np.searchsorted(cdf, cdf[-1] // 2)]

  def mode_(val, freq):
    return val[np.argmax(freq)]

  def var_(val, freq):
    avg = mean_(val, freq)
    dev = freq * (val - avg) ** 2
    return dev.sum() / (freq.sum() - 1)

  def std_(val, freq):
    return np.sqrt(var_(val, freq))

  print('AND gaps')
  print('\t# gaps: %d' % and_gap.shape[0])
  gap, f = and_gap[:, 0], and_gap[:, 1]
  min_gap, max_gap = np.min(gap), np.max(gap)
  print('\tmin gap: %d' % min_gap)
  print('\tmax gap: %d' % max_gap)
  mean_gap, median_gap = mean_(gap, f), median_(gap, f)
  print('\tmedian gap: %d' % median_gap)
  print('\tmean gap: %d' % int(mean_gap))
  mode_gap, std_gap = mode_(gap, f), std_(gap, f)
  print('\tmode: %d' % mode_gap)
  print('\tstd: %.1f' % std_gap)

  print('XOR gaps')
  print('\t# gaps: %d' % xor_gap.shape[0])
  gap, f = xor_gap[:, 0], xor_gap[:, 1]
  min_gap, max_gap = np.min(gap), np.max(gap)
  print('\tmin gap: %d' % min_gap)
  print('\tmax gap: %d' % max_gap)
  mean_gap, median_gap = mean_(gap, f), median_(gap, f)
  print('\tmedian gap: %d' % median_gap)
  print('\tmean gap: %d' % int(mean_gap))
  mode_gap, std_gap = mode_(gap, f), std_(gap, f)
  print('\tmode: %d' % mode_gap)
  print('\tstd: %.1f' % std_gap)


if __name__ == '__main__':
  if len(sys.argv) < 2:
    print('Provide filepath to stats file as input')
    sys.exit(1)

  main(sys.argv[1])
