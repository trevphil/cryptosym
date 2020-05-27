VERBOSE = True

VISUALIZE = True

ENTRY_POINT = 0 # 0 = do everything, 1 = build undirected graph, 2 = directed graph, 3 = factor graph

HASH_INPUT_NBITS = 64

HASH_MODE = 'pseudo_hash'

HASH_MODES = [
  'sha256',
  'md5',
  'map_from_input', # each hash bit is equivalent to a bit in the input message
  'conditioned_on_input', # use simple CPDs conditioned on a single hash input bit
  'conditioned_on_input_and_hash', # use more complex CPDs conditioned on hash and input msg
  'pseudo_hash' # fake hashing algorithm but with mathematical operations
]

EPSILON = 1e-4

LBP_CONVERGENCE_THRESH = 1e-4

LBP_MAX_ITER = 20

MAX_CONNECTIONS_PER_NODE = 5

BIT_PRED = 256 + 0 # a.k.a. the first bit of the input message

DATASET_SIZE = 20000

DATASET_PATH = './data/data.csv'

PROB_DATA_PATH = './data/prob.npy'

UDG_DATA_PATH = './data/bn_undirected.yaml'

DG_DATA_PATH = './data/bn_directed.yaml'
