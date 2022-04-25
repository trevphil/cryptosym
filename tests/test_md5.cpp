/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#include <gtest/gtest.h>
#include <string>
#include <boost/dynamic_bitset.hpp>
#include <boost/algorithm/string.hpp>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/dll.h>
#include <cryptopp/md5.h>

#include "core/utils.hpp"
#include "hashing/sym_md5.hpp"

namespace preimage {

TEST(MD5Test, RandomInputsAndSizes) {
  utils::seed(1);
  const std::vector<int> inp_sizes{0, 8, 32, 64, 512, 640, 1024};
  for (int inp_size : inp_sizes) {
    MD5 md5(inp_size);

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

      // Call MD5 using Crypto++
      CryptoPP::Weak::MD5 cryptopp_md5;
      uint8_t digest[CryptoPP::Weak::MD5::DIGESTSIZE];
      cryptopp_md5.CalculateDigest(digest, byte_arr, n_bytes);

      // Convert output to lowercase hexadecimal
      CryptoPP::HexEncoder encoder;
      std::string expected_output;
      encoder.Attach(new CryptoPP::StringSink(expected_output));
      encoder.Put(digest, sizeof(digest));
      encoder.MessageEnd();
      boost::algorithm::to_lower(expected_output);

      // Call MD5 using custom hash function
      const std::string h = utils::hexstr(md5.call(bits));
      EXPECT_EQ(h, expected_output);
    }
  }
}

}  // end namespace preimage