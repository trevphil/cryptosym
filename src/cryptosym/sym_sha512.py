"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

from ._cpp import SymBitVec, SymHash

_K = (
    SymBitVec(0x428A2F98D728AE22, size=64),
    SymBitVec(0x7137449123EF65CD, size=64),
    SymBitVec(0xB5C0FBCFEC4D3B2F, size=64),
    SymBitVec(0xE9B5DBA58189DBBC, size=64),
    SymBitVec(0x3956C25BF348B538, size=64),
    SymBitVec(0x59F111F1B605D019, size=64),
    SymBitVec(0x923F82A4AF194F9B, size=64),
    SymBitVec(0xAB1C5ED5DA6D8118, size=64),
    SymBitVec(0xD807AA98A3030242, size=64),
    SymBitVec(0x12835B0145706FBE, size=64),
    SymBitVec(0x243185BE4EE4B28C, size=64),
    SymBitVec(0x550C7DC3D5FFB4E2, size=64),
    SymBitVec(0x72BE5D74F27B896F, size=64),
    SymBitVec(0x80DEB1FE3B1696B1, size=64),
    SymBitVec(0x9BDC06A725C71235, size=64),
    SymBitVec(0xC19BF174CF692694, size=64),
    SymBitVec(0xE49B69C19EF14AD2, size=64),
    SymBitVec(0xEFBE4786384F25E3, size=64),
    SymBitVec(0x0FC19DC68B8CD5B5, size=64),
    SymBitVec(0x240CA1CC77AC9C65, size=64),
    SymBitVec(0x2DE92C6F592B0275, size=64),
    SymBitVec(0x4A7484AA6EA6E483, size=64),
    SymBitVec(0x5CB0A9DCBD41FBD4, size=64),
    SymBitVec(0x76F988DA831153B5, size=64),
    SymBitVec(0x983E5152EE66DFAB, size=64),
    SymBitVec(0xA831C66D2DB43210, size=64),
    SymBitVec(0xB00327C898FB213F, size=64),
    SymBitVec(0xBF597FC7BEEF0EE4, size=64),
    SymBitVec(0xC6E00BF33DA88FC2, size=64),
    SymBitVec(0xD5A79147930AA725, size=64),
    SymBitVec(0x06CA6351E003826F, size=64),
    SymBitVec(0x142929670A0E6E70, size=64),
    SymBitVec(0x27B70A8546D22FFC, size=64),
    SymBitVec(0x2E1B21385C26C926, size=64),
    SymBitVec(0x4D2C6DFC5AC42AED, size=64),
    SymBitVec(0x53380D139D95B3DF, size=64),
    SymBitVec(0x650A73548BAF63DE, size=64),
    SymBitVec(0x766A0ABB3C77B2A8, size=64),
    SymBitVec(0x81C2C92E47EDAEE6, size=64),
    SymBitVec(0x92722C851482353B, size=64),
    SymBitVec(0xA2BFE8A14CF10364, size=64),
    SymBitVec(0xA81A664BBC423001, size=64),
    SymBitVec(0xC24B8B70D0F89791, size=64),
    SymBitVec(0xC76C51A30654BE30, size=64),
    SymBitVec(0xD192E819D6EF5218, size=64),
    SymBitVec(0xD69906245565A910, size=64),
    SymBitVec(0xF40E35855771202A, size=64),
    SymBitVec(0x106AA07032BBD1B8, size=64),
    SymBitVec(0x19A4C116B8D2D0C8, size=64),
    SymBitVec(0x1E376C085141AB53, size=64),
    SymBitVec(0x2748774CDF8EEB99, size=64),
    SymBitVec(0x34B0BCB5E19B48A8, size=64),
    SymBitVec(0x391C0CB3C5C95A63, size=64),
    SymBitVec(0x4ED8AA4AE3418ACB, size=64),
    SymBitVec(0x5B9CCA4F7763E373, size=64),
    SymBitVec(0x682E6FF3D6B2B8A3, size=64),
    SymBitVec(0x748F82EE5DEFB2FC, size=64),
    SymBitVec(0x78A5636F43172F60, size=64),
    SymBitVec(0x84C87814A1F0AB72, size=64),
    SymBitVec(0x8CC702081A6439EC, size=64),
    SymBitVec(0x90BEFFFA23631E28, size=64),
    SymBitVec(0xA4506CEBDE82BDE9, size=64),
    SymBitVec(0xBEF9A3F7B2C67915, size=64),
    SymBitVec(0xC67178F2E372532B, size=64),
    SymBitVec(0xCA273ECEEA26619C, size=64),
    SymBitVec(0xD186B8C721C0C207, size=64),
    SymBitVec(0xEADA7DD6CDE0EB1E, size=64),
    SymBitVec(0xF57D4F7FEE6ED178, size=64),
    SymBitVec(0x06F067AA72176FBA, size=64),
    SymBitVec(0x0A637DC5A2C898A6, size=64),
    SymBitVec(0x113F9804BEF90DAE, size=64),
    SymBitVec(0x1B710B35131C471B, size=64),
    SymBitVec(0x28DB77F523047D84, size=64),
    SymBitVec(0x32CAAB7B40C72493, size=64),
    SymBitVec(0x3C9EBE0A15C9BEBC, size=64),
    SymBitVec(0x431D67C49C100D4C, size=64),
    SymBitVec(0x4CC5D4BECB3E42B6, size=64),
    SymBitVec(0x597F299CFC657E2A, size=64),
    SymBitVec(0x5FCB6FAB3AD6FAEC, size=64),
    SymBitVec(0x6C44198C4A475817, size=64),
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
            SymBitVec(0x6A09E667F3BCC908, size=64),
            SymBitVec(0xBB67AE8584CAA73B, size=64),
            SymBitVec(0x3C6EF372FE94F82B, size=64),
            SymBitVec(0xA54FF53A5F1D36F1, size=64),
            SymBitVec(0x510E527FADE682D1, size=64),
            SymBitVec(0x9B05688C2B3E6C1F, size=64),
            SymBitVec(0x1F83D9ABFB41BD6B, size=64),
            SymBitVec(0x5BE0CD19137E2179, size=64),
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
                s0 = SymBitVec.xor3(
                    w[i - 15].rotr(1), w[i - 15].rotr(8), w[i - 15] >> 7
                )
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
