# Description

This repository contains Python and C++ code which attempts to reverse one-way cryptographic hash functions, with specific focus on [SHA-256](https://en.bitcoinwiki.org/wiki/SHA-256). A hash function `f` can be thought of as an operation on bits `X` to produce output bits `Y`: `f(X) = Y`. Given knowledge of `Y` and how `f` works, we want to recover `X`. This is commonly known as a [pre-image attack](https://en.wikipedia.org/wiki/Preimage_attack).

A successful pre-image attack has serious implications for basically the entire Internet, financial community, and national defense of major governments. Hash functions are used in all kinds of domains: from BitCoin mining and transactions, to HTTPS encryption, to storage of user passwords in server databases.

I've spent over a year trying to solve this "impossible" problem using a variety of methods, detailed below. As a **disclaimer**: I do not claim that any of these methods break the security of the full 64-round SHA-256 hash function. It's probably still infeasible. Prove me wrong :)

# Installation

It is not necessary to install everything listed here. If you just want to run certain parts of the code (e.g. C++ vs. Python) you can skip some steps. I developed everything on a Mac, so most things will translate well to Linux but unfortunately not to Windows.

### Python
Use Python 3.6 with [Anaconda](https://docs.anaconda.com/anaconda/install/). Start by creating and activating a new environment:

```
conda create -n preimage python=3.6
conda activate preimage
python --version  # Should show 3.6
```

Install project dependencies:

```
pip install -r requirements.txt
```

Not all of the solving methods will work out-of-the-box from here. If you want certain solvers, you will need to install them individually:

- [Gurobi](https://www.gurobi.com/)
- [Cplex](https://www.ibm.com/analytics/cplex-optimizer)
- [MiniSat](http://minisat.se/)
- [CryptoMiniSat](https://www.msoos.org/cryptominisat5)

For Cplex on Mac OS X, make sure to do:

```
export PYTHONPATH=/Applications/CPLEX_Studio1210/cplex/python/3.6/x86-64_osx
```

### C++

You need to install [cmake](https://cmake.org/install/) and [`cpp-yaml`](https://github.com/jbeder/yaml-cpp). Then to compile the code in this repository, nagivate to the [`belief_propagation`](./belief_propagation) directory and run:

```
$ ./kompile
```

# Quickstart

Generate datasets:

```
$ ./generate_all_hashes.sh
```

Choose a dataset:

```
$ ls -la ./data
drwxr-xr-x  27 trevphil  staff  864 Nov  3 23:04 .
drwxr-xr-x  15 trevphil  staff  480 Nov 22 12:29 ..
drwxr-xr-x  11 trevphil  staff  352 Nov  3 22:54 addConst_d1
drwxr-xr-x  11 trevphil  staff  352 Nov  3 22:53 add_d1
drwxr-xr-x  11 trevphil  staff  352 Nov  3 22:54 andConst_d1
drwxr-xr-x  10 trevphil  staff  320 Nov  3 22:54 sha256_d1
...
```

The "d1" at the end means that the difficulty level is "1". Run a solver on the dataset of your choice:

```
$ python -m optimization.main data/addConst_d1 --solver ortools_cp
```

To see the available solvers and usage of the tool:

```
$ python -m optimization.main --help
usage: main.py [-h]
               [--solver {gradient,gnc,cplex_milp,cplex_cp,ortools_cp,ortools_milp,gurobi_milp,minisat,crypto_minisat}]
               dataset

Hash reversal via optimization

positional arguments:
  dataset               Path to the dataset directory

optional arguments:
  -h, --help            Show this help message and exit
  --solver {gradient,gnc,cplex_milp,cplex_cp,ortools_cp,ortools_milp,gurobi_milp,minisat,crypto_minisat}
                        The solving technique
```

### Writing your own hash function

Take a look at the existing hash functions in [`dataset_generation/hash_funcs.py`](./dataset_generation/hash_funcs.py). For example, this function simply adds a constant value to the input:

```
class AddConst(SymbolicHash):
    def hash(self, hash_input: SymBitVec, difficulty: int):
        n = len(hash_input)
        A = SymBitVec(0x4F65D4D99B70EF1B, size=n)
        return hash_input + A
```

Implement your own hash function class which inherits from `SymbolicHash` and has the function `hash(...)`, and add your class to the dictionary returned by the function `hash_algorithms()` in [`dataset_generation/hash_funcs.py`](./dataset_generation/hash_funcs.py). The following operations are supported for primitives (`SymBitVec` objects) in the hash function:

- AND: `C = A & B`
- OR: `C = A | B`
- XOR: `C = A ^ B`
- NOT (aka INV for "inverse"): `B = ~A`
- Addition: `C = A + B`
	- `A`, `B`, and `C` should have the same number of bits, overflow is ignored
- Shift left: `B = (A << n)`
	- `B` will have the same number of bits as `A`
- Shift right: `B = (A >> n)`
	- `B` will have the same number of bits as `A`

To generate a dataset using your hash function, run a command like the following:

```
$ python -m dataset_generation.generate --num-samples 64 --num-input-bits 64 --hash-algo my_custom_hash --visualize --difficulty 4
```

The dataset is explain more in depth below. To see the usage of the dataset generation tool, run:

```
python -m dataset_generation.generate --help
usage: generate.py [-h] [--data-dir DATA_DIR] [--num-samples NUM_SAMPLES]
                   [--num-input-bits NUM_INPUT_BITS]
                   [--hash-algo {add,addConst,andConst,invert,lossyPseudoHash,nonLossyPseudoHash,orConst,sha256,shiftLeft,shiftRight,xorConst}]
                   [--difficulty DIFFICULTY] [--visualize]
                   [--hash-input HASH_INPUT] [--pct-val PCT_VAL]
                   [--pct-test PCT_TEST]

Hash reversal dataset generator

optional arguments:
  -h, --help            Show this help message and exit
  --data-dir DATA_DIR   Path to the directory where the dataset should be
                        stored
  --num-samples NUM_SAMPLES
                        Number of samples to use in the dataset
  --num-input-bits NUM_INPUT_BITS
                        Number of bits in each input message to the hash
                        function
  --hash-algo {add,addConst,andConst,invert,lossyPseudoHash,nonLossyPseudoHash,orConst,sha256,shiftLeft,shiftRight,xorConst}
                        Choose the hashing algorithm to apply to the input
                        data
  --difficulty DIFFICULTY
                        SHA-256 difficulty (an interger between 1 and 64
                        inclusive)
  --visualize           Visualize the symbolic graph of bit dependencies
  --hash-input HASH_INPUT
                        Give input message in hex to simply print the hash of
                        the input
  --pct-val PCT_VAL     Percent of samples used for validation dataset
  --pct-test PCT_TEST   Percent of samples used for test dataset
``` 

# How it works

### Dataset generation

The first thing we need in order to approach this problem is to find out how the hash function `f` operates on input bits `X` to produce output bits `Y` without making any assumptions about `X`. To this end, I make the hash functions operate on symbolic bit vectors, i.e. `SymBitVec` objects. These objects track the relationship between input and output bits for all computations in the hash function, e.g. if `C = A & B`, we track the relationship that bit `C` is the result of AND-ing `A` and `B`.

Some simplifications can be made during this process. For example, let's say that bit `A` is a bit from the unknown input `X` and bit `B` is a _constant_ in the hash algorithm equal to 0. Well then we know that `C = A & 0 = 0` so `C = 0` no matter the value of `A`. Some more simplifications for operations on single bits are listed below:

- `B = A & 1 = A`
- `B = A | 0 = A`
- `B = A | 1 = 1`
- `B = A ^ 1 = ~A`
- `B = A ^ 0 = A`
- `B = A ^ A = 0`
- `B = A & A = A | A = A`

These simplifications help to reduce the size of the symbolic representation of the hash function, since the output bit `B` is sometimes a constant or equal to the unknown input `A`. When this happens, we don't need to introduce a new unknown variable. 

Furthermore, the problem can be made easier to handle by reducing all operations (XOR, OR, addition) to only using AND and INV logic gates. For example, `C = A ^ B` is equivalent to:

```
X = ~(A & B)
Y = ~(A & X)
Z = ~(B & X)
C = ~(Y & Z)
```

This introduces intermediate variables, but critically, the AND and INV operations can be **linearized** and also represented in the continuous domain, which is important mathematically. Normally in the discrete domain, we would consider each bit as a binary "[random variable](https://en.wikipedia.org/wiki/Random_variable)" taking the value 0 or 1.

The AND operation can be represented with multiplication: `C = A & B = A * B`, and the INV operation with subtraction: `B = ~A = 1 - A`. To linearize the AND operation, we can use [the following method](https://or.stackexchange.com/questions/37/how-to-linearize-the-product-of-two-binary-variables):

```
C = A & B = A * B
Equivalent to:
C <= A
C <= B
C >= A + B - 1
```

Note that no information is lost during the INV operation (we can always recover the input from the output of INV), but there _is_ information lost during AND when the output is 0. When the output is 1, we know that both input must have been 1. But when the output is 0, there are three possible inputs: `0 = 0 & 0 = 0 & 1 = 1 & 0`. **Thus, all complexity in reversing a hash function comes from AND gates whose output is 0**.

### Dataset Format

TODO