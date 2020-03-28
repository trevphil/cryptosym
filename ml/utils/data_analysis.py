# -*- coding: utf-8 -*-
import numpy as np
from numpy import loadtxt
from scipy import stats

dataset = loadtxt('data.csv', delimiter=',')
means = np.mean(dataset, axis=0)
stds = np.std(dataset, axis=0)
N = dataset.shape[0]

# If the hash bits are truly random, we should expect that
# 50% of the time each bit is 0, and 50% of the time each bit is 1.
# The variance of each bit should be 1/4 (i.e. standard deviation = 1/2).
#
# We perform a t-test for each bit to determine if the bit's sample
# distribution is equal to the expected distribution (mean=0.5)...

print('********** t-test **********')

num_improbable = 0

for variable in range(dataset.shape[1]):
  # null hypothesis = "mean is 0.5"
  # alternative hypothesis = "mean is not 0.5"
  # if p < 0.05 we reject the null hypothesis --> "mean is not 0.5"
  stat, p = stats.ttest_1samp(dataset[:, variable], 0.5)
  if abs(stat) > 2 and p < 0.05:
    print('Mean of bit %d is probably not 0.5' % variable)
    num_improbable += 1

print('num_improbable = %d' % num_improbable)
assert num_improbable > 0, 'I am crazy, cannot derive a relationship between bits in a sha256 hash'
# But if you re-generate the dataset, the bits whose mean is not 0.5
# are not consistent across randomly-generated data sets... :(
