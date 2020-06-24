from os import path
from pathlib import Path

# 0 = do everything
# 1 = make undirected graph
# 2 = undirected graph already cached, make factor graph
ENTRY_POINT = 0

HASH_INPUT_NBITS = 64

EPSILON = 1e-4

LBP_MAX_ITER = 10

MAX_CONNECTIONS_PER_NODE = 6

BIT_PRED = 256 + 0 # a.k.a. the first bit of the input message

DATA_DIR = path.abspath('./data')

UDG_DATA_FILE = path.join(DATA_DIR, 'bn_undirected.yaml')

def makeDataDirectoryIfNeeded():
  Path(DATA_DIR).mkdir(parents=True, exist_ok=True)