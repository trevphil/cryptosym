from os import path
from math import log
from time import localtime, strftime
from pathlib import Path

VERBOSE = True

VISUALIZE = True

# 0 = do everything
# 1 = need to calculate mutual information scores
# 2 = mutual info scores already calculated, make undirected graph
# 3 = undirected graph already cached, make factor graph
ENTRY_POINT = 0

HASH_INPUT_NBITS = 64

HASH_MODE = 'sha256'

HASH_MODES = [
  'sha256',
  'md5',
  'map_from_input', # each hash bit is equivalent to a bit in the input message
  'conditioned_on_input', # use simple CPDs conditioned on a single hash input bit
  'conditioned_on_input_and_hash', # use more complex CPDs conditioned on hash and input msg
  'pseudo_hash' # fake hashing algorithm but with mathematical operations
]

EPSILON = 1e-4

LBP_MAX_ITER = 10

MAX_CONNECTIONS_PER_NODE = 16

BIT_PRED = 256 + 0 # a.k.a. the first bit of the input message

PROB_ALL_CPDS_IN_DATASET = 0.95

# Dataset size is computed such that for any configuration of (MAX_CONNECTIONS_PER_NODE + 1)
# combinations of binary random variables, there is a probability of PROB_ALL_CPDS_IN_DATASET
# that at least one item in the dataset has that configuration of 0's and 1's.
DATASET_SIZE = int(log(1.0 - PROB_ALL_CPDS_IN_DATASET) / log(1.0 - pow(2.0, -(MAX_CONNECTIONS_PER_NODE + 1))))

EXPERIMENT_DIR = path.join(path.abspath('./experiments'),
                           HASH_MODE + '_' + strftime('%Y-%m-%d-%H-%M-%S', localtime()))

DATA_DIR = path.abspath('./data')

DATASET_FILE = path.join(DATA_DIR, 'data.csv')

PROB_DATA_FILE = path.join(DATA_DIR, 'prob.npy')

FCG_DATA_FILE = path.join(DATA_DIR, 'bn_fully_connected.yaml')

UDG_DATA_FILE = path.join(DATA_DIR, 'bn_undirected.yaml')

def makeDataDirectoryIfNeeded():
  Path(DATA_DIR).mkdir(parents=True, exist_ok=True)

def makeExperimentDirectoryIfNeeded():
  Path(EXPERIMENT_DIR).mkdir(parents=True, exist_ok=True)
