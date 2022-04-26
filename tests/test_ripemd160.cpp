/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <cryptopp/dll.h>
#include <cryptopp/ripemd.h>
#include <gtest/gtest.h>

#include <boost/algorithm/string.hpp>
#include <boost/dynamic_bitset.hpp>
#include <string>

#include "core/utils.hpp"
#include "hashing/sym_ripemd160.hpp"

namespace preimage {

TEST(RIPEMD160Test, RandomInputsAndSizes) {
  utils::seed(1);
  const std::vector<int> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (int inp_size : inp_sizes) {
    RIPEMD160 ripemd160(inp_size);

    for (int sample_idx = 0; sample_idx < 10; sample_idx++) {
      // Generate a random input of length "inp_size"
      const boost::dynamic_bitset<> bits = utils::randomBits(inp_size);
      const int n_bytes = inp_size / 8;
      uint8_t byte_arr[n_bytes];
      for (int byte_idx = 0; byte_idx < n_bytes; byte_idx++) {
        byte_arr[byte_idx] = 0;
        for (int k = 0; k < 8; k++) {
          byte_arr[byte_idx] += bits[(byte_idx * 8) + k] << k;
        }
      }

      // Call RIPEMD160 using Crypto++
      CryptoPP::RIPEMD160 cryptopp_ripemd160;
      uint8_t digest[CryptoPP::RIPEMD160::DIGESTSIZE];
      cryptopp_ripemd160.CalculateDigest(digest, byte_arr, n_bytes);

      // Convert output to lowercase hexadecimal
      CryptoPP::HexEncoder encoder;
      std::string expected_output;
      encoder.Attach(new CryptoPP::StringSink(expected_output));
      encoder.Put(digest, sizeof(digest));
      encoder.MessageEnd();
      boost::algorithm::to_lower(expected_output);

      // Call RIPEMD160 using custom hash function
      const std::string h = utils::hexstr(ripemd160.call(bits));
      EXPECT_EQ(h, expected_output);
    }
  }
}

}  // end namespace preimage