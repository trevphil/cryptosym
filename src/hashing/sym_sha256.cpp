/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "hashing/sym_sha256.hpp"

#include "core/utils.hpp"

namespace preimage {

#define Ch(x, y, z) (z ^ (x & (y ^ z)))
#define Maj(x, y, z) (SymBitVec::majority3(x, y, z))
#define S(x, n) ((x >> (n & 31)) | (x << (32 - (n & 31))))
#define R(x, n) (x >> n)
#define Sigma0(x) (SymBitVec::xor3(S(x, 2), S(x, 13), S(x, 22)))
#define Sigma1(x) (SymBitVec::xor3(S(x, 6), S(x, 11), S(x, 25)))
#define Gamma0(x) (SymBitVec::xor3(S(x, 7), S(x, 18), R(x, 3)))
#define Gamma1(x) (SymBitVec::xor3(S(x, 17), S(x, 19), R(x, 10)))

SHA256::SHA256() {
  block_size_ = 64;
  digest_size_ = 32;
  const std::vector<uint32_t> raw_words = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
      0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
      0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
      0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
      0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
      0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
      0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
      0xc67178f2};
  words_ = {};
  for (uint32_t w : raw_words) {
    words_.push_back(SymBitVec(w, 32));
  }
  resetState();
}

void SHA256::resetState() {
  local_ = 0;
  count_lo_ = 0;
  count_hi_ = 0;
  digest_ = {SymBitVec(0x6A09E667, 32), SymBitVec(0xBB67AE85, 32),
             SymBitVec(0x3C6EF372, 32), SymBitVec(0xA54FF53A, 32),
             SymBitVec(0x510E527F, 32), SymBitVec(0x9B05688C, 32),
             SymBitVec(0x1F83D9AB, 32), SymBitVec(0x5BE0CD19, 32)};
  w_ = {};
  data_ = {};
  for (int i = 0; i < block_size_; i++) {
    data_.push_back(SymBitVec(0, 8));
  }
}

std::pair<SymBitVec, SymBitVec> SHA256::round(const SymBitVec &a, const SymBitVec &b,
                                              const SymBitVec &c, const SymBitVec &d,
                                              const SymBitVec &e, const SymBitVec &f,
                                              const SymBitVec &g, const SymBitVec &h,
                                              int i, const SymBitVec &ki) {
  const SymBitVec t0 = h + Sigma1(e) + Ch(e, f, g) + ki + w_[i];
  const SymBitVec t1 = Sigma0(a) + Maj(a, b, c);
  return {d + t0, t0 + t1};
}

void SHA256::transform(int difficulty) {
  w_ = {};
  std::vector<SymBitVec> d;
  for (const SymBitVec &bv : data_) d.push_back(bv.resize(32));
  for (int i = 0; i < 16; ++i) {
    w_.push_back((d[4 * i] << 24) + (d[4 * i + 1] << 16) + (d[4 * i + 2] << 8) +
                 d[4 * i + 3]);
  }
  for (int i = 16; i < 64; ++i) {
    w_.push_back(Gamma1(w_[i - 2]) + w_[i - 7] + Gamma0(w_[i - 15]) + w_[i - 16]);
  }

  std::vector<SymBitVec> ss = digest_;
  std::vector<int> i = {0, 1, 2, 3, 4, 5, 6, 7};
  for (int j = 0; j < difficulty && j < 64; ++j) {
    const auto output = round(ss[i[0]], ss[i[1]], ss[i[2]], ss[i[3]], ss[i[4]], ss[i[5]],
                              ss[i[6]], ss[i[7]], j, words_.at(j));
    ss[i[3]] = output.first;
    ss[i[7]] = output.second;
    std::rotate(i.begin(), i.begin() + 7, i.end());  // rotate right by 1
  }

  for (int j = 0; j < digest_.size(); j++) {
    digest_[j] = digest_.at(j) + ss.at(j);
  }
}

void SHA256::update(const SymBitVec &bv, int difficulty) {
  int count = bv.size() / 8;
  int buffer_idx = 0;
  int clo = (count_lo_ + (count << 3)) & 0xffffffff;
  if (clo < count_lo_) ++count_hi_;
  count_lo_ = clo;
  count_hi_ += (count >> 29);

  if (local_) {
    int i = block_size_ - local_;
    if (i > count) i = count;

    const int n_bytes = i;
    for (int byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
      const int bit_lower = (buffer_idx + byte_idx) * 8;
      const int bit_upper = bit_lower + 8;
      data_[local_ + byte_idx] = bv.extract(bit_lower, bit_upper);
    }

    count -= i;
    buffer_idx += i;
    local_ += i;
    if (local_ == block_size_) {
      transform(difficulty);
      local_ = 0;
    } else {
      return;
    }
  }

  while (count >= block_size_) {
    data_ = {};
    for (int byte_idx = 0; byte_idx < block_size_; byte_idx++) {
      const int bit_lower = (buffer_idx + byte_idx) * 8;
      const int bit_upper = bit_lower + 8;
      data_.push_back(bv.extract(bit_lower, bit_upper));
    }

    count -= block_size_;
    buffer_idx += block_size_;
    transform(difficulty);
  }

  for (int byte_idx = 0; byte_idx < count; byte_idx++) {
    const int bit_lower = (buffer_idx + byte_idx) * 8;
    const int bit_upper = bit_lower + 8;
    data_[local_ + byte_idx] = bv.extract(bit_lower, bit_upper);
  }
  local_ = count;
}

SymBitVec SHA256::digest(int difficulty) {
  int count = (count_lo_ >> 3) & 0x3f;
  data_[count] = SymBitVec(0x80, 8);
  count++;
  const SymBitVec zero(0, 8);
  if (count > block_size_ - 8) {
    data_.resize(count);
    for (int i = 0; i < block_size_ - count; ++i) data_.push_back(zero);

    transform(difficulty);

    data_ = {};
    for (int i = 0; i < block_size_; ++i) data_.push_back(zero);
  } else {
    data_.resize(count);
    for (int i = 0; i < block_size_ - count; ++i) data_.push_back(zero);
  }

  const SymBitVec lo_bit_count(count_lo_, 32);
  const SymBitVec hi_bit_count(count_hi_, 32);
  data_[56] = (hi_bit_count >> 24).resize(8);
  data_[57] = (hi_bit_count >> 16).resize(8);
  data_[58] = (hi_bit_count >> 8).resize(8);
  data_[59] = (hi_bit_count >> 0).resize(8);
  data_[60] = (lo_bit_count >> 24).resize(8);
  data_[61] = (lo_bit_count >> 16).resize(8);
  data_[62] = (lo_bit_count >> 8).resize(8);
  data_[63] = (lo_bit_count >> 0).resize(8);

  transform(difficulty);

  std::vector<SymBitVec> dig = {};
  for (const SymBitVec &i : digest_) {
    dig.push_back((i >> 24).resize(8));
    dig.push_back((i >> 16).resize(8));
    dig.push_back((i >> 8).resize(8));
    dig.push_back(i.resize(8));
    if (dig.size() >= digest_size_) break;
  }

  SymBitVec result = dig.at(0);
  for (int i = 1; i < dig.size(); ++i) {
    result = dig.at(i).concat(result);
  }
  return result;
}

SymBitVec SHA256::hash(const SymBitVec &hash_input, int difficulty) {
  resetState();
  update(hash_input, difficulty);
  return digest(difficulty);
}

#undef Ch
#undef Maj
#undef S
#undef R
#undef Sigma0
#undef Sigma1
#undef Gamma0
#undef Gamma1

}  // end namespace preimage
