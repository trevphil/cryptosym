/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/ripemd.h>
#include <gtest/gtest.h>
#include <stdlib.h>

#include <algorithm>
#include <cctype>
#include <string>

#include "core/bit_vec.hpp"
#include "core/utils.hpp"
#include "hashing/sym_ripemd160.hpp"

namespace preimage {

TEST(SymRIPEMD160Test, InputSizeMismatch) {
  SymRIPEMD160 ripemd160(32);
  const BitVec inputs = utils::randomBits(64);
  EXPECT_THROW({ ripemd160.call(inputs); }, std::length_error);
}

TEST(SymRIPEMD160Test, BadInputSize) {
  EXPECT_THROW({ SymRIPEMD160(31); }, std::length_error);
}

TEST(SymRIPEMD160Test, RandomInputsAndSizes) {
  utils::seed(1);
  const std::vector<unsigned int> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (unsigned int inp_size : inp_sizes) {
    SymRIPEMD160 ripemd160(inp_size);

    for (int sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size"
      const BitVec bits = utils::randomBits(inp_size);
      const unsigned int n_bytes = inp_size / 8;
      uint8_t *byte_arr = reinterpret_cast<uint8_t *>(calloc(n_bytes, sizeof(uint8_t)));
      for (unsigned int byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (unsigned int k = 0; k < 8; k++) {
          byte_arr[byte_idx] += bits[(byte_idx * 8) + k] << k;
        }
      }

      // Call RIPEMD160 using Crypto++
      CryptoPP::RIPEMD160 cryptopp_ripemd160;
      uint8_t digest[CryptoPP::RIPEMD160::DIGESTSIZE];
      cryptopp_ripemd160.CalculateDigest(digest, byte_arr, n_bytes);
      free(byte_arr);

      // Convert output to lowercase hexadecimal
      CryptoPP::HexEncoder encoder;
      std::string expected_output;
      encoder.Attach(new CryptoPP::StringSink(expected_output));
      encoder.Put(digest, sizeof(digest));
      encoder.MessageEnd();
      std::transform(expected_output.begin(), expected_output.end(),
                     expected_output.begin(),
                     [](unsigned char c) -> unsigned char { return std::tolower(c); });

      // Call RIPEMD160 using custom hash function
      const std::string h = utils::hexstr(ripemd160.call(bits));
      EXPECT_EQ(h, expected_output);
    }
  }
}

}  // end namespace preimage