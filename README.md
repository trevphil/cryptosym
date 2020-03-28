# Contents

- [Disclaimers](#disclaimers)
- [Description](#description)
- [Dependencies and Installation](#dependencies-and-installation)
- [Deterministic SHA-256 Reversal](#deterministic-sha-256-reversal)
- [Probabilistic SHA-256 Reversal](#probabilistic-sha-256-reversal)
- [Deterministic MD5 Reversal](#deterministic-md5-reversal)
- [Future Work and Extensions](#future-work-and-extensions)
- [References and Resources](#references-and-resources)

# Disclaimers

1. The implementation of this code works in theory, but I do not claim that it breaks the security of any cryptographic hash functions. To reverse an entire hash, it may still require infinite compute. I tested this code on high-performant Google Cloud servers; after 1 week of running, the program did not terminate or yield a successful pre-image attack.

2. I wrote this code without doing research on methods other people have tried for breaking cryptographic hash functions. This is because I wanted to have fresh, creative ideas without being influenced by prior research. However I did some research afterwards, and a list of relevant papers and topics are provided at the end of the README.

3. I am not an expert in cryptography or BitCoin.

# Description

This repository contains my attempts at pre-image attacks on SHA-256, MD5, and BitCoin. It tries to solve the following questions **deterministically** (with a SAT solver) and **probabilistically** (with Bayesian networks and loopy belief propagation):

- Given a SHA-256 hash, can one find the input used to generate the hash?
- Given a partially known hash input and constraints on the (unknown) hash output, can the unknown section of the hash input be recovered?
- Given a SHA-256 hash, can a single bit of the hash input be predicted with high accuracy?


# Dependencies and Installation

I recommend using [Anaconda](https://www.anaconda.com/) to run the code in this project. It's the easiest way I've found to install the [z3 prover](https://github.com/Z3Prover/z3) for Python.

Use Python 3.5 and install from `requirements.txt`:

```
conda create -n hash_reversal -python=3.5
conda activate hash_reversal
pip install -r requirements.txt --user
```

# Deterministic SHA-256 Reversal

### Explanation

The idea here is to build the entire SHA-256 function symbolically, set constraints on the input and output of the hash function, and then feed this problem to a satisfiability (SAT) solver. It turns out that this approach has already been extensively researched for breaking cryptographic hash functions [1], [2], [3].

Instead of the hash input being a bit stream, it is a _variable_ of fixed length. Some of the bits in the input may be known and other unknown. For example, in a (simplified) BitCoin, the first 608 bits of a block are known and the remaining 32 bits (the nonce) are unknown. The block is fed to SHA-256 twice, and the resulting hash should be less than some pre-defined value (the "[difficulty](https://en.bitcoin.it/wiki/Difficulty)"). From this we can form constraints on our problem: only the 32-bit nonce is an unknown input variable, and the output is constrained to have a certain number of zeroes in the least-significant bits (LSB).

The SAT solver I chose is the [z3 Theorem Prover](https://github.com/Z3Prover/z3) from Microsoft Research. I used the [BitVector](https://z3prover.github.io/api/html/classz3py_1_1_bit_vec_ref.html) primative from this library, which represents a bit stream symbolically.

The problem can be formulated as follows:
```
```

For a great explanation of how SAT solvers work, I recommend [1].

### A note on endianness

Keeping track of endianness for this type of problem is a huge headache. A [Big Endian](https://en.wikipedia.org/wiki/Endianness#Big-endian) number looks something like this:
```
```

A [Litte Endian](https://en.wikipedia.org/wiki/Endianness#Little-endian) number looks like this:
```
```

### How to run

The code for this section is in the `sha256` directory. You can run tests with `python test.py`. Be prepared to wait _at least_ until the death of the Sun for the tests to finish :)

For more reasonable tests you may adjust test parameters. For example: reduce the number of bits in the hash input message, reduce the required number of leading zeros in the hash, run the input through SHA-256 once (not twice), or even add a hash as a solver constraint and verify that the correct input message used to generate the hash is returned.

The tests are currently configured to predict the 32-bit nonce for the [Genesis block](https://en.bitcoin.it/wiki/Genesis_block) of BitCoin, given the other 608 bits. This requires 64 zeros in the hash and two passes through SHA-256, which is not the easiest of problems for a SAT solver...

# Probabilistic SHA-256 Reversal

### Dataset

The probabilistic approaches require a dataset. I formed a dataset by generating random 64-bit messages, feeding them to SHA-256, and writing the output to a CSV file. 

Each line in the CSV file consists of 256 + 64 = 320 values, each 0 or 1. The first 256 entries are the SHA-256 hash and the remaining 64 are the hash input.

The script to create a dataset is [`generate_dataset.py`](./ml/generate_dataset.py).

### Neural network

Neural networks boil down to complex function approximators. So maybe it is reasonable that we can approximate `reverse(SHA256(message))` with a neural network.

See [`train_ml.py`](./ml/train_ml.py) for my attempt at doing this with TensorFlow. The network attempts to predict a single bit of the input message, given all bits of the SHA-256 hash.

In short, it doesn't work--at least not with the simple network architectures I tried out. Test accuracy and loss don't budge, they are constant.

p.s. neural networks like this one are technically **deterministic**, not probabilistic, I know :)

### Bayesian network

The neural network approach was doomed to fail in my opinion. It would require an enormously complex model to even begin to approximate a SHA-256 reversal function.

I think the more interesting approach lies in exploiting [conditional probability distributions](https://en.wikipedia.org/wiki/Conditional_probability_distribution) (CPDs) between hash input and output bits. For example, what is the probability of the input message's first bit being 1, given that hash bits 5, 32, and 67 are 0?

For a perfect hash function, the probability of any hash bit being 1 or 0 should be completely independent from other hash bits and from the input message. However this is real life, and I think it's reasonable to assume that there exist _extremely small_ CPDs between bits. The question is, how to exploit them?

To attempt to exploit CPDs, I built a [Bayesian network](https://en.wikipedia.org/wiki/Bayesian_network) from the dataset, converted it to a [factor graph](https://en.wikipedia.org/wiki/Factor_graph), performed loopy belief propagation [4], and used the converged message values for inference.

**Spoiler alert**: although 50.3% prediction accuracy may be statistically significant given 20,000 samples, it is not useful. In the end, this approach was also not successful.

The idea for this "attack" came after taking the course [Probabilistic Artificial Intelligence](https://las.inf.ethz.ch/pai-f19) at ETH Zürich.

### How to run

The code for this section is in the `ml` directory. You can run everything with `python main.py`, but make sure you've generated the dataset first.

The first thing that will happen is that mutual information scores beteen each random variable will be generated, then an undirected Bayesian network will be made, then directions will be added to the graph, then a factor graph will be constructed from the directed graph. For each sample in the dataset, loopy belief propagation will run until convergence, and the result will be used to infer a single bit of the hash input message.

# Deterministic MD5 Reversal

### Explanation

Before trying to break SHA-256 with a SAT solver, I wanted to test out the approach on the "already broken" MD5 hash function. Basically everything I've written in [Deterministic SHA-256 Reversal](#deterministic-sha-256-reversal) also applies here.

The regular MD5 implementation comes from [5].

### How to run

The code for this section is in the `md5` directory. You can run tests with `python test.py`.

# Future Work and Extensions

# References and Resources

- [1] https://www.duo.uio.no/bitstream/handle/10852/34912/thesis-output.pdf?sequence=1&isAllowed=y

- [2] http://jheusser.github.io/2013/02/03/satcoin.html

- [3] https://www.microsoft.com/en-us/research/publication/applications-of-sat-solvers-to-cryptanalysis-of-hash-functions/

- [4] http://nghiaho.com/?page_id=1366

- [5] https://github.com/narkkil/md5

