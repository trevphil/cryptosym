/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include "hashing/sym_ripemd160.hpp"

#include "core/config.hpp"

#define RIPEMD160_SIZE 160

namespace preimage {

#define Subround(f, a, b, c, d, e, x, s, k) \
  a = a + f(b, c, d) + x + k;               \
  a = rotateLeft(a, s) + e;                 \
  c = rotateLeft(c, 10);                    \
  i++;                                      \
  if (i >= difficulty_) return

RIPEMD160::RIPEMD160(int num_input_bits, int difficulty)
    : SymHash(num_input_bits, difficulty) {
  if (difficulty_ < 0) difficulty_ = defaultDifficulty();
  if (config::verbose) {
    printf("Initialized %s with difficulty %d\n", hashName().c_str(), difficulty_);
  }

  k0_ = SymBitVec(0, 32);
  k1_ = SymBitVec(0x5a827999, 32);
  k2_ = SymBitVec(0x6ed9eba1, 32);
  k3_ = SymBitVec(0x8f1bbcdc, 32);
  k4_ = SymBitVec(0xa953fd4e, 32);
  k5_ = SymBitVec(0x50a28be6, 32);
  k6_ = SymBitVec(0x5c4dd124, 32);
  k7_ = SymBitVec(0x6d703ef3, 32);
  k8_ = SymBitVec(0x7a6d76e9, 32);
  k9_ = SymBitVec(0, 32);
}

SymBitVec RIPEMD160::hash(const SymBitVec &hash_input) {
  const int n_bytes = hash_input.size() / 8;

  resetState();

  // Process message in chunks of 64 bytes
  int bit_index = 0;
  for (int b = n_bytes; b > 63; b -= 64) {
    for (int i = 0; i < 16; i++) {
      X_[i] = hash_input.extract(bit_index, bit_index + 32);
      bit_index += 32;
    }
    transform();
  }

  // Process the final chunk of the input message
  finalize(hash_input, bit_index, n_bytes, 0);

  SymBitVec hashcode[20];  // Each SymBitVec in `hashcode` has 8 bits
  for (int i = 0; i < RIPEMD160_SIZE / 8; i += 4) {
    hashcode[i + 0] = (buffer_[i >> 2] >> 0).extract(0, 8);
    hashcode[i + 1] = (buffer_[i >> 2] >> 8).extract(0, 8);
    hashcode[i + 2] = (buffer_[i >> 2] >> 16).extract(0, 8);
    hashcode[i + 3] = (buffer_[i >> 2] >> 24).extract(0, 8);
  }

  SymBitVec combined_digest;
  for (int i = 0; i < 20; i++) {
    combined_digest = hashcode[i].concat(combined_digest);
  }
  return combined_digest;
}

void RIPEMD160::resetState() {
  buffer_[0] = SymBitVec(0x67452301, 32);
  buffer_[1] = SymBitVec(0xefcdab89, 32);
  buffer_[2] = SymBitVec(0x98badcfe, 32);
  buffer_[3] = SymBitVec(0x10325476, 32);
  buffer_[4] = SymBitVec(0xc3d2e1f0, 32);
}

void RIPEMD160::finalize(const SymBitVec &hash_input, int bit_index, int lo, int hi) {
  for (int i = 0; i < 16; i++) X_[i] = SymBitVec(0, 32);

  SymBitVec tmp;
  for (int i = 0; i < (lo & 63); i++) {
    tmp = hash_input.extract(bit_index, bit_index + 8);
    tmp = (tmp.resize(32) << (8 * (i & 3)));
    X_[i >> 2] = X_[i >> 2] ^ tmp;
    bit_index += 8;
  }

  tmp = SymBitVec(1 << (8 * (lo & 3) + 7), 32);
  X_[(lo >> 2) & 15] = X_[(lo >> 2) & 15] ^ tmp;

  if ((lo & 63) > 55) {
    transform();
    for (int i = 0; i < 16; i++) X_[i] = SymBitVec(0, 32);
  }

  X_[14] = SymBitVec(lo << 3, 32);
  X_[15] = SymBitVec((lo >> 29) | (hi << 3), 32);
  transform();
}

void RIPEMD160::transformInternal(SymBitVec &a1, SymBitVec &a2, SymBitVec &b1,
                                  SymBitVec &b2, SymBitVec &c1, SymBitVec &c2,
                                  SymBitVec &d1, SymBitVec &d2, SymBitVec &e1,
                                  SymBitVec &e2) {
  int i = 0;

  /* Round 1 */
  Subround(F, a1, b1, c1, d1, e1, X_[0], 11, k0_);
  Subround(F, e1, a1, b1, c1, d1, X_[1], 14, k0_);
  Subround(F, d1, e1, a1, b1, c1, X_[2], 15, k0_);
  Subround(F, c1, d1, e1, a1, b1, X_[3], 12, k0_);
  Subround(F, b1, c1, d1, e1, a1, X_[4], 5, k0_);
  Subround(F, a1, b1, c1, d1, e1, X_[5], 8, k0_);
  Subround(F, e1, a1, b1, c1, d1, X_[6], 7, k0_);
  Subround(F, d1, e1, a1, b1, c1, X_[7], 9, k0_);
  Subround(F, c1, d1, e1, a1, b1, X_[8], 11, k0_);
  Subround(F, b1, c1, d1, e1, a1, X_[9], 13, k0_);
  Subround(F, a1, b1, c1, d1, e1, X_[10], 14, k0_);
  Subround(F, e1, a1, b1, c1, d1, X_[11], 15, k0_);
  Subround(F, d1, e1, a1, b1, c1, X_[12], 6, k0_);
  Subround(F, c1, d1, e1, a1, b1, X_[13], 7, k0_);
  Subround(F, b1, c1, d1, e1, a1, X_[14], 9, k0_);
  Subround(F, a1, b1, c1, d1, e1, X_[15], 8, k0_);

  /* Round 2 */
  Subround(G, e1, a1, b1, c1, d1, X_[7], 7, k1_);
  Subround(G, d1, e1, a1, b1, c1, X_[4], 6, k1_);
  Subround(G, c1, d1, e1, a1, b1, X_[13], 8, k1_);
  Subround(G, b1, c1, d1, e1, a1, X_[1], 13, k1_);
  Subround(G, a1, b1, c1, d1, e1, X_[10], 11, k1_);
  Subround(G, e1, a1, b1, c1, d1, X_[6], 9, k1_);
  Subround(G, d1, e1, a1, b1, c1, X_[15], 7, k1_);
  Subround(G, c1, d1, e1, a1, b1, X_[3], 15, k1_);
  Subround(G, b1, c1, d1, e1, a1, X_[12], 7, k1_);
  Subround(G, a1, b1, c1, d1, e1, X_[0], 12, k1_);
  Subround(G, e1, a1, b1, c1, d1, X_[9], 15, k1_);
  Subround(G, d1, e1, a1, b1, c1, X_[5], 9, k1_);
  Subround(G, c1, d1, e1, a1, b1, X_[2], 11, k1_);
  Subround(G, b1, c1, d1, e1, a1, X_[14], 7, k1_);
  Subround(G, a1, b1, c1, d1, e1, X_[11], 13, k1_);
  Subround(G, e1, a1, b1, c1, d1, X_[8], 12, k1_);

  /* Round 3 */
  Subround(H, d1, e1, a1, b1, c1, X_[3], 11, k2_);
  Subround(H, c1, d1, e1, a1, b1, X_[10], 13, k2_);
  Subround(H, b1, c1, d1, e1, a1, X_[14], 6, k2_);
  Subround(H, a1, b1, c1, d1, e1, X_[4], 7, k2_);
  Subround(H, e1, a1, b1, c1, d1, X_[9], 14, k2_);
  Subround(H, d1, e1, a1, b1, c1, X_[15], 9, k2_);
  Subround(H, c1, d1, e1, a1, b1, X_[8], 13, k2_);
  Subround(H, b1, c1, d1, e1, a1, X_[1], 15, k2_);
  Subround(H, a1, b1, c1, d1, e1, X_[2], 14, k2_);
  Subround(H, e1, a1, b1, c1, d1, X_[7], 8, k2_);
  Subround(H, d1, e1, a1, b1, c1, X_[0], 13, k2_);
  Subround(H, c1, d1, e1, a1, b1, X_[6], 6, k2_);
  Subround(H, b1, c1, d1, e1, a1, X_[13], 5, k2_);
  Subround(H, a1, b1, c1, d1, e1, X_[11], 12, k2_);
  Subround(H, e1, a1, b1, c1, d1, X_[5], 7, k2_);
  Subround(H, d1, e1, a1, b1, c1, X_[12], 5, k2_);

  /* Round 4 */
  Subround(I, c1, d1, e1, a1, b1, X_[1], 11, k3_);
  Subround(I, b1, c1, d1, e1, a1, X_[9], 12, k3_);
  Subround(I, a1, b1, c1, d1, e1, X_[11], 14, k3_);
  Subround(I, e1, a1, b1, c1, d1, X_[10], 15, k3_);
  Subround(I, d1, e1, a1, b1, c1, X_[0], 14, k3_);
  Subround(I, c1, d1, e1, a1, b1, X_[8], 15, k3_);
  Subround(I, b1, c1, d1, e1, a1, X_[12], 9, k3_);
  Subround(I, a1, b1, c1, d1, e1, X_[4], 8, k3_);
  Subround(I, e1, a1, b1, c1, d1, X_[13], 9, k3_);
  Subround(I, d1, e1, a1, b1, c1, X_[3], 14, k3_);
  Subround(I, c1, d1, e1, a1, b1, X_[7], 5, k3_);
  Subround(I, b1, c1, d1, e1, a1, X_[15], 6, k3_);
  Subround(I, a1, b1, c1, d1, e1, X_[14], 8, k3_);
  Subround(I, e1, a1, b1, c1, d1, X_[5], 6, k3_);
  Subround(I, d1, e1, a1, b1, c1, X_[6], 5, k3_);
  Subround(I, c1, d1, e1, a1, b1, X_[2], 12, k3_);

  /* Round 5 */
  Subround(J, b1, c1, d1, e1, a1, X_[4], 9, k4_);
  Subround(J, a1, b1, c1, d1, e1, X_[0], 15, k4_);
  Subround(J, e1, a1, b1, c1, d1, X_[5], 5, k4_);
  Subround(J, d1, e1, a1, b1, c1, X_[9], 11, k4_);
  Subround(J, c1, d1, e1, a1, b1, X_[7], 6, k4_);
  Subround(J, b1, c1, d1, e1, a1, X_[12], 8, k4_);
  Subround(J, a1, b1, c1, d1, e1, X_[2], 13, k4_);
  Subround(J, e1, a1, b1, c1, d1, X_[10], 12, k4_);
  Subround(J, d1, e1, a1, b1, c1, X_[14], 5, k4_);
  Subround(J, c1, d1, e1, a1, b1, X_[1], 12, k4_);
  Subround(J, b1, c1, d1, e1, a1, X_[3], 13, k4_);
  Subround(J, a1, b1, c1, d1, e1, X_[8], 14, k4_);
  Subround(J, e1, a1, b1, c1, d1, X_[11], 11, k4_);
  Subround(J, d1, e1, a1, b1, c1, X_[6], 8, k4_);
  Subround(J, c1, d1, e1, a1, b1, X_[15], 5, k4_);
  Subround(J, b1, c1, d1, e1, a1, X_[13], 6, k4_);

  /* Parallel round 1 */
  Subround(J, a2, b2, c2, d2, e2, X_[5], 8, k5_);
  Subround(J, e2, a2, b2, c2, d2, X_[14], 9, k5_);
  Subround(J, d2, e2, a2, b2, c2, X_[7], 9, k5_);
  Subround(J, c2, d2, e2, a2, b2, X_[0], 11, k5_);
  Subround(J, b2, c2, d2, e2, a2, X_[9], 13, k5_);
  Subround(J, a2, b2, c2, d2, e2, X_[2], 15, k5_);
  Subround(J, e2, a2, b2, c2, d2, X_[11], 15, k5_);
  Subround(J, d2, e2, a2, b2, c2, X_[4], 5, k5_);
  Subround(J, c2, d2, e2, a2, b2, X_[13], 7, k5_);
  Subround(J, b2, c2, d2, e2, a2, X_[6], 7, k5_);
  Subround(J, a2, b2, c2, d2, e2, X_[15], 8, k5_);
  Subround(J, e2, a2, b2, c2, d2, X_[8], 11, k5_);
  Subround(J, d2, e2, a2, b2, c2, X_[1], 14, k5_);
  Subround(J, c2, d2, e2, a2, b2, X_[10], 14, k5_);
  Subround(J, b2, c2, d2, e2, a2, X_[3], 12, k5_);
  Subround(J, a2, b2, c2, d2, e2, X_[12], 6, k5_);

  /* Parallel round 2 */
  Subround(I, e2, a2, b2, c2, d2, X_[6], 9, k6_);
  Subround(I, d2, e2, a2, b2, c2, X_[11], 13, k6_);
  Subround(I, c2, d2, e2, a2, b2, X_[3], 15, k6_);
  Subround(I, b2, c2, d2, e2, a2, X_[7], 7, k6_);
  Subround(I, a2, b2, c2, d2, e2, X_[0], 12, k6_);
  Subround(I, e2, a2, b2, c2, d2, X_[13], 8, k6_);
  Subround(I, d2, e2, a2, b2, c2, X_[5], 9, k6_);
  Subround(I, c2, d2, e2, a2, b2, X_[10], 11, k6_);
  Subround(I, b2, c2, d2, e2, a2, X_[14], 7, k6_);
  Subround(I, a2, b2, c2, d2, e2, X_[15], 7, k6_);
  Subround(I, e2, a2, b2, c2, d2, X_[8], 12, k6_);
  Subround(I, d2, e2, a2, b2, c2, X_[12], 7, k6_);
  Subround(I, c2, d2, e2, a2, b2, X_[4], 6, k6_);
  Subround(I, b2, c2, d2, e2, a2, X_[9], 15, k6_);
  Subround(I, a2, b2, c2, d2, e2, X_[1], 13, k6_);
  Subround(I, e2, a2, b2, c2, d2, X_[2], 11, k6_);

  /* Parallel round 3 */
  Subround(H, d2, e2, a2, b2, c2, X_[15], 9, k7_);
  Subround(H, c2, d2, e2, a2, b2, X_[5], 7, k7_);
  Subround(H, b2, c2, d2, e2, a2, X_[1], 15, k7_);
  Subround(H, a2, b2, c2, d2, e2, X_[3], 11, k7_);
  Subround(H, e2, a2, b2, c2, d2, X_[7], 8, k7_);
  Subround(H, d2, e2, a2, b2, c2, X_[14], 6, k7_);
  Subround(H, c2, d2, e2, a2, b2, X_[6], 6, k7_);
  Subround(H, b2, c2, d2, e2, a2, X_[9], 14, k7_);
  Subround(H, a2, b2, c2, d2, e2, X_[11], 12, k7_);
  Subround(H, e2, a2, b2, c2, d2, X_[8], 13, k7_);
  Subround(H, d2, e2, a2, b2, c2, X_[12], 5, k7_);
  Subround(H, c2, d2, e2, a2, b2, X_[2], 14, k7_);
  Subround(H, b2, c2, d2, e2, a2, X_[10], 13, k7_);
  Subround(H, a2, b2, c2, d2, e2, X_[0], 13, k7_);
  Subround(H, e2, a2, b2, c2, d2, X_[4], 7, k7_);
  Subround(H, d2, e2, a2, b2, c2, X_[13], 5, k7_);

  /* Parallel round 4 */
  Subround(G, c2, d2, e2, a2, b2, X_[8], 15, k8_);
  Subround(G, b2, c2, d2, e2, a2, X_[6], 5, k8_);
  Subround(G, a2, b2, c2, d2, e2, X_[4], 8, k8_);
  Subround(G, e2, a2, b2, c2, d2, X_[1], 11, k8_);
  Subround(G, d2, e2, a2, b2, c2, X_[3], 14, k8_);
  Subround(G, c2, d2, e2, a2, b2, X_[11], 14, k8_);
  Subround(G, b2, c2, d2, e2, a2, X_[15], 6, k8_);
  Subround(G, a2, b2, c2, d2, e2, X_[0], 14, k8_);
  Subround(G, e2, a2, b2, c2, d2, X_[5], 6, k8_);
  Subround(G, d2, e2, a2, b2, c2, X_[12], 9, k8_);
  Subround(G, c2, d2, e2, a2, b2, X_[2], 12, k8_);
  Subround(G, b2, c2, d2, e2, a2, X_[13], 9, k8_);
  Subround(G, a2, b2, c2, d2, e2, X_[9], 12, k8_);
  Subround(G, e2, a2, b2, c2, d2, X_[7], 5, k8_);
  Subround(G, d2, e2, a2, b2, c2, X_[10], 15, k8_);
  Subround(G, c2, d2, e2, a2, b2, X_[14], 8, k8_);

  /* Parallel round 5 */
  Subround(F, b2, c2, d2, e2, a2, X_[12], 8, k9_);
  Subround(F, a2, b2, c2, d2, e2, X_[15], 5, k9_);
  Subround(F, e2, a2, b2, c2, d2, X_[10], 12, k9_);
  Subround(F, d2, e2, a2, b2, c2, X_[4], 9, k9_);
  Subround(F, c2, d2, e2, a2, b2, X_[1], 12, k9_);
  Subround(F, b2, c2, d2, e2, a2, X_[5], 5, k9_);
  Subround(F, a2, b2, c2, d2, e2, X_[8], 14, k9_);
  Subround(F, e2, a2, b2, c2, d2, X_[7], 6, k9_);
  Subround(F, d2, e2, a2, b2, c2, X_[6], 8, k9_);
  Subround(F, c2, d2, e2, a2, b2, X_[2], 13, k9_);
  Subround(F, b2, c2, d2, e2, a2, X_[13], 6, k9_);
  Subround(F, a2, b2, c2, d2, e2, X_[14], 5, k9_);
  Subround(F, e2, a2, b2, c2, d2, X_[0], 15, k9_);
  Subround(F, d2, e2, a2, b2, c2, X_[3], 13, k9_);
  Subround(F, c2, d2, e2, a2, b2, X_[9], 11, k9_);
  Subround(F, b2, c2, d2, e2, a2, X_[11], 11, k9_);
}

void RIPEMD160::transform() {
  SymBitVec a1 = buffer_[0];
  SymBitVec a2 = buffer_[0];
  SymBitVec b1 = buffer_[1];
  SymBitVec b2 = buffer_[1];
  SymBitVec c1 = buffer_[2];
  SymBitVec c2 = buffer_[2];
  SymBitVec d1 = buffer_[3];
  SymBitVec d2 = buffer_[3];
  SymBitVec e1 = buffer_[4];
  SymBitVec e2 = buffer_[4];

  transformInternal(a1, a2, b1, b2, c1, c2, d1, d2, e1, e2);

  c1 = buffer_[1] + c1 + d2;
  buffer_[1] = buffer_[2] + d1 + e2;
  buffer_[2] = buffer_[3] + e1 + a2;
  buffer_[3] = buffer_[4] + a1 + b2;
  buffer_[4] = buffer_[0] + b1 + c2;
  buffer_[0] = c1;
}

}  // end namespace preimage
