import sys
import numpy as np
from matplotlib import pyplot as plt

dist = np.loadtxt('/tmp/bp_dist.txt', delimiter=',').T
bits = np.loadtxt('/tmp/bp_bits.txt', delimiter=',').T

fig = plt.figure()
ax1 = fig.add_subplot(121)
ax2 = fig.add_subplot(122)

p = ax1.matshow(dist, aspect='auto', cmap='jet')
plt.colorbar(p, ax=ax1)

p = ax2.matshow(bits, aspect='auto', cmap='jet')
plt.colorbar(p, ax=ax2)

fig.set_size_inches(14, 8)
if len(sys.argv) > 1:
  plt.savefig(sys.argv[1])
  print('Saved figure to %s' % sys.argv[1])
plt.show()
