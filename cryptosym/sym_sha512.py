"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

from ._cpp import SymBitVec, SymHash

_K = (
    SymBitVec(0x428a2f98d728ae22, size=64),
    SymBitVec(0x7137449123ef65cd, size=64),
    SymBitVec(0xb5c0fbcfec4d3b2f, size=64),
    SymBitVec(0xe9b5dba58189dbbc, size=64),
    SymBitVec(0x3956c25bf348b538, size=64),
    SymBitVec(0x59f111f1b605d019, size=64),
    SymBitVec(0x923f82a4af194f9b, size=64),
    SymBitVec(0xab1c5ed5da6d8118, size=64),
    SymBitVec(0xd807aa98a3030242, size=64),
    SymBitVec(0x12835b0145706fbe, size=64),
    SymBitVec(0x243185be4ee4b28c, size=64),
    SymBitVec(0x550c7dc3d5ffb4e2, size=64),
    SymBitVec(0x72be5d74f27b896f, size=64),
    SymBitVec(0x80deb1fe3b1696b1, size=64),
    SymBitVec(0x9bdc06a725c71235, size=64),
    SymBitVec(0xc19bf174cf692694, size=64),
    SymBitVec(0xe49b69c19ef14ad2, size=64),
    SymBitVec(0xefbe4786384f25e3, size=64),
    SymBitVec(0x0fc19dc68b8cd5b5, size=64),
    SymBitVec(0x240ca1cc77ac9c65, size=64),
    SymBitVec(0x2de92c6f592b0275, size=64),
    SymBitVec(0x4a7484aa6ea6e483, size=64),
    SymBitVec(0x5cb0a9dcbd41fbd4, size=64),
    SymBitVec(0x76f988da831153b5, size=64),
    SymBitVec(0x983e5152ee66dfab, size=64),
    SymBitVec(0xa831c66d2db43210, size=64),
    SymBitVec(0xb00327c898fb213f, size=64),
    SymBitVec(0xbf597fc7beef0ee4, size=64),
    SymBitVec(0xc6e00bf33da88fc2, size=64),
    SymBitVec(0xd5a79147930aa725, size=64),
    SymBitVec(0x06ca6351e003826f, size=64),
    SymBitVec(0x142929670a0e6e70, size=64),
    SymBitVec(0x27b70a8546d22ffc, size=64),
    SymBitVec(0x2e1b21385c26c926, size=64),
    SymBitVec(0x4d2c6dfc5ac42aed, size=64),
    SymBitVec(0x53380d139d95b3df, size=64),
    SymBitVec(0x650a73548baf63de, size=64),
    SymBitVec(0x766a0abb3c77b2a8, size=64),
    SymBitVec(0x81c2c92e47edaee6, size=64),
    SymBitVec(0x92722c851482353b, size=64),
    SymBitVec(0xa2bfe8a14cf10364, size=64),
    SymBitVec(0xa81a664bbc423001, size=64),
    SymBitVec(0xc24b8b70d0f89791, size=64),
    SymBitVec(0xc76c51a30654be30, size=64),
    SymBitVec(0xd192e819d6ef5218, size=64),
    SymBitVec(0xd69906245565a910, size=64),
    SymBitVec(0xf40e35855771202a, size=64),
    SymBitVec(0x106aa07032bbd1b8, size=64),
    SymBitVec(0x19a4c116b8d2d0c8, size=64),
    SymBitVec(0x1e376c085141ab53, size=64),
    SymBitVec(0x2748774cdf8eeb99, size=64),
    SymBitVec(0x34b0bcb5e19b48a8, size=64),
    SymBitVec(0x391c0cb3c5c95a63, size=64),
    SymBitVec(0x4ed8aa4ae3418acb, size=64),
    SymBitVec(0x5b9cca4f7763e373, size=64),
    SymBitVec(0x682e6ff3d6b2b8a3, size=64),
    SymBitVec(0x748f82ee5defb2fc, size=64),
    SymBitVec(0x78a5636f43172f60, size=64),
    SymBitVec(0x84c87814a1f0ab72, size=64),
    SymBitVec(0x8cc702081a6439ec, size=64),
    SymBitVec(0x90befffa23631e28, size=64),
    SymBitVec(0xa4506cebde82bde9, size=64),
    SymBitVec(0xbef9a3f7b2c67915, size=64),
    SymBitVec(0xc67178f2e372532b, size=64),
    SymBitVec(0xca273eceea26619c, size=64),
    SymBitVec(0xd186b8c721c0c207, size=64),
    SymBitVec(0xeada7dd6cde0eb1e, size=64),
    SymBitVec(0xf57d4f7fee6ed178, size=64),
    SymBitVec(0x06f067aa72176fba, size=64),
    SymBitVec(0x0a637dc5a2c898a6, size=64),
    SymBitVec(0x113f9804bef90dae, size=64),
    SymBitVec(0x1b710b35131c471b, size=64),
    SymBitVec(0x28db77f523047d84, size=64),
    SymBitVec(0x32caab7b40c72493, size=64),
    SymBitVec(0x3c9ebe0a15c9bebc, size=64),
    SymBitVec(0x431d67c49c100d4c, size=64),
    SymBitVec(0x4cc5d4becb3e42b6, size=64),
    SymBitVec(0x597f299cfc657e2a, size=64),
    SymBitVec(0x5fcb6fab3ad6faec, size=64),
    SymBitVec(0x6c44198c4a475817, size=64),
)


class SymSHA512(SymHash):
    def __init__(self, num_input_bits: int, difficulty: int = -1):
        SymHash.__init__(self, num_input_bits, difficulty)
        self.reset()

    def hash_name(self) -> str:
        return "SHA512"

    def default_difficulty(self) -> int:
        return 80

    def reset(self) -> None:
        self._h = [
            SymBitVec(0x6a09e667f3bcc908, size=64),
            SymBitVec(0xbb67ae8584caa73b, size=64),
            SymBitVec(0x3c6ef372fe94f82b, size=64),
            SymBitVec(0xa54ff53a5f1d36f1, size=64),
            SymBitVec(0x510e527fade682d1, size=64),
            SymBitVec(0x9b05688c2b3e6c1f, size=64),
            SymBitVec(0x1f83d9abfb41bd6b, size=64),
            SymBitVec(0x5be0cd19137e2179, size=64),
        ]

    def forward(self, hash_input: SymBitVec) -> SymBitVec:
        self.reset()

        hash_input = hash_input.reversed_bytes()
        self.update(hash_input)

        output = SymBitVec()
        for h in self._h:
            output = h.concat(output)
        return output

    def update(self, data: SymBitVec) -> None:
        h0, h1, h2, h3, h4, h5, h6, h7 = self._h

        orig_len_in_bits = len(data) & 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
        orig_len_in_bits = orig_len_in_bits.to_bytes(16, byteorder="little")

        data = SymBitVec(0x80, size=8).concat(data)

        while data.num_bytes % 128 != 112:
            data = SymBitVec(0, size=8).concat(data)

        data = SymBitVec(orig_len_in_bits).concat(data)
        assert data.num_bytes % 128 == 0, "Error in padding"

        for byte_index in range(data.num_bytes - 128, -1, -128):
            bit_index_lower = byte_index * 8
            bit_index_upper = (byte_index + 128) * 8
            chunk = data[bit_index_lower:bit_index_upper]
            w = [SymBitVec(0, size=64) for i in range(80)]

            for i in range(16):
                lower = i * 64
                upper = (i + 1) * 64
                w[i] = chunk[lower:upper]
            w[0:16] = w[0:16][::-1]

            for i in range(16, 80):
                s0 = SymBitVec.xor3(w[i - 15].rotr(1), w[i - 15].rotr(8), w[i - 15] >> 7)
                a = w[i - 2].rotr(19)
                b = w[i - 2].rotr(61)
                c = w[i - 2] >> 6
                s1 = SymBitVec.xor3(a, b, c)
                w[i] = w[i - 16] + s0 + w[i - 7] + s1

            a, b, c, d, e, f, g, h = h0, h1, h2, h3, h4, h5, h6, h7

            for i in range(min(self.difficulty, 80)):
                S1 = SymBitVec.xor3(e.rotr(14), e.rotr(18), e.rotr(41))
                ch = (e & f) ^ ((~e) & g)
                temp1 = h + S1 + ch + _K[i] + w[i]
                S0 = SymBitVec.xor3(a.rotr(28), a.rotr(34), a.rotr(39))
                maj = SymBitVec.maj3(a, b, c)
                temp2 = S0 + maj
                new_a = temp1 + temp2
                new_e = d + temp1
                a, b, c, d, e, f, g, h = new_a, a, b, c, new_e, e, f, g

            h0 = h0 + a
            h1 = h1 + b
            h2 = h2 + c
            h3 = h3 + d
            h4 = h4 + e
            h5 = h5 + f
            h6 = h6 + g
            h7 = h7 + h

        self._h = [h0, h1, h2, h3, h4, h5, h6, h7]
