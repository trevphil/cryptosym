# Endianness

## Endianness of bits

`SymBitVec` objects use **little Endian**, that is, the least-significant bit (LSB)
is stored first (index 0) in the sequence and the most-significant bit (MSB) last.

## Endianness of bytes

As `SymBitVec` objects work at the individual bit level and use little Endian,
(i.e. LSB at index 0) bytes are formed by taking 8-bit chunks of a bit vector and thus,
**byte representations are also little Endian**.

### Concatenation

Calling `a.concat(b)` will place `b`'s bits _after_ `a`'s.
The LSB of `b` will be placed at one index _greater_ than the MSB of `a`.

```
a = 0b000000
b = 0b111111
                           | index 0
a.concat(b) = 0b111111000000
                ^          ^
               MSB        LSB
```

### Rotate right/left

We read numbers from left to right, and the `rotr` / `rotl` functions of `SymBitVec`
follow our natural intuition despite `SymBitVec`'s little Endian representation.
Below, we demonstrate left/right rotation for a 3-bit example.

```
a = 0b101
  value  1  0  1
  index  2  1  0

a.rotr(1) = 0b110
  value  1  1  0
  index  2  1  0

a.rotl(1) = 0b011
  value  0  1  1
  index  2  1  0
```

# License

This work is licensed under Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0).
For the official version, please refer to [LICENSE.md](/LICENSE.md). A summary is provided below:

You are free to:

- **Share** - copy and redistribute the material in any medium or format
- **Adapt** - remix, transform, and build upon the material

Under the following terms:

- **Attribution** - You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
- **NonCommercial** - You may not use the material for commercial purposes.
- **ShareAlike** - If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original.
- **No additional restrictions** - You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.

Notice:

- You do not have to comply with the license for elements of the material in the public domain or where your use is permitted by an applicable exception or limitation.
- No warranties are given. The license may not give you all of the permissions necessary for your intended use. For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.
