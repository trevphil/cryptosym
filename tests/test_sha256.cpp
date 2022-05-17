/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * Distributed under the CC BY-NC-SA 4.0 license
 * (See accompanying file LICENSE.md).
 */

#include <cryptlib.h>
#include <gtest/gtest.h>
#include <hex.h>
#include <sha.h>
#include <stdlib.h>

#include <algorithm>
#include <cctype>
#include <string>

#include "core/bit_vec.hpp"
#include "core/utils.hpp"
#include "hashing/sym_sha256.hpp"

namespace preimage {

TEST(SHA256Test, EmptyInput) {
  const std::string empty = "";
  const std::string expected =
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

  SHA256 sha256(0);

  std::string h = utils::hexstr(sha256.call(utils::str2bits(empty)));
  EXPECT_EQ(h, expected);
}

TEST(SHA256Test, FixedInput) {
  const std::string s = "just a test string";
  const std::string s7 = s + s + s + s + s + s + s;
  const std::string expected =
      "8113ebf33c97daa9998762aacafe750c7cefc2b2f173c90c59663a57fe626f21";

  SHA256 sha256(static_cast<int>(s7.size() * 8));

  std::string h = utils::hexstr(sha256.call(utils::str2bits(s7)));
  EXPECT_EQ(h, expected);
}

TEST(SHA256Test, InputSizeMismatch) {
  SHA256 sha256(32);
  const BitVec inputs = utils::randomBits(64);
  EXPECT_THROW({ sha256.call(inputs); }, std::length_error);
}

TEST(SHA256Test, BadInputSize) {
  EXPECT_THROW({ SHA256(31); }, std::length_error);
}

TEST(SHA256Test, RandomInputsAndSizes) {
  utils::seed(1);
  const std::vector<unsigned int> inp_sizes{8, 32, 64, 512, 640, 1024};
  for (unsigned int inp_size : inp_sizes) {
    SHA256 sha256(inp_size);

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

      // Call SHA256 using Crypto++
      CryptoPP::SHA256 cryptopp_sha256;
      uint8_t digest[CryptoPP::SHA256::DIGESTSIZE];
      cryptopp_sha256.CalculateDigest(digest, byte_arr, n_bytes);
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

      // Call SHA256 using custom hash function
      const std::string h = utils::hexstr(sha256.call(bits));
      EXPECT_EQ(h, expected_output);
    }
  }
}

}  // end namespace preimage
