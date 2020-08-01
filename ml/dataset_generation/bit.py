# -*- coding: utf-8 -*-
from dataset_generation.factor import Factor, FactorType


class Bit(object):
  rv_index = 0
  factors = []
  rv_bits = []

  def __init__(self, bit_value, is_rv):
    self.val = bool(bit_value)
    self.is_rv = is_rv
    if self.is_rv:
      self.index = Bit.rv_index
      Bit.rv_index += 1
      Bit.rv_bits.append(self)

  @staticmethod
  def reset():
    Bit.rv_index = 0
    Bit.factors = []
    Bit.rv_bits = []
  
  def __repr__(self):
    if self.is_rv:
      return 'Bit({}, index: {})'.format(int(self.val), self.index)
    else:
      return 'Bit({}, constant)'.format(int(self.val))

  def __invert__(a):
    result_val = not bool(a.val)
    is_rv = a.is_rv
    result = Bit(result_val, is_rv)

    if a.is_rv:
      f_type = FactorType.INV
      Bit.factors.append(Factor(f_type, result, [a]))
    
    return result

  def __xor__(a, b):
    result_val = a.val ^ b.val
    is_rv = a.is_rv or b.is_rv
    result = Bit(result_val, is_rv)

    if a.is_rv and b.is_rv:
      f_type = FactorType.XOR
      Bit.factors.append(Factor(f_type, result, [a, b]))
    elif a.is_rv:
      f_type = FactorType.XOR_C1 if b.val is True else FactorType.XOR_C0
      Bit.factors.append(Factor(f_type, result, [a]))
    elif b.is_rv:
      f_type = FactorType.XOR_C1 if a.val is True else FactorType.XOR_C0
      Bit.factors.append(Factor(f_type, result, [b]))

    return result
  
  def __or__(a, b):
    result_val = a.val | b.val
    is_rv = a.is_rv or b.is_rv
    result = Bit(result_val, is_rv)

    if a.is_rv and b.is_rv:
      f_type = FactorType.OR
      Bit.factors.append(Factor(f_type, result, [a, b]))
    elif a.is_rv:
      f_type = FactorType.OR_C1 if b.val is True else FactorType.OR_C0
      Bit.factors.append(Factor(f_type, result, [a]))
    elif b.is_rv:
      f_type = FactorType.OR_C1 if a.val is True else FactorType.OR_C0
      Bit.factors.append(Factor(f_type, result, [b]))

    return result
  
  def __and__(a, b):
    result_val = a.val & b.val
    is_rv = a.is_rv or b.is_rv
    result = Bit(result_val, is_rv)

    if a.is_rv and b.is_rv:
      f_type = FactorType.AND
      Bit.factors.append(Factor(f_type, result, [a, b]))
    elif a.is_rv:
      f_type = FactorType.AND_C1 if b.val is True else FactorType.AND_C0
      Bit.factors.append(Factor(f_type, result, [a]))
    elif b.is_rv:
      f_type = FactorType.AND_C1 if a.val is True else FactorType.AND_C0
      Bit.factors.append(Factor(f_type, result, [b]))

    return result
  
  @staticmethod
  def add(a, b, carry=None):
    if carry is None:
      carry = Bit(0, False)

    out = int(a.val) + int(b.val) + int(carry.val)
    result_val = bool(out & 1)
    carry_out_val = bool((out >> 1) & 1)
    
    is_rv = a.is_rv or b.is_rv or carry.is_rv
    result = Bit(result_val, is_rv)

    f_types = {
      tuple(): FactorType.ADD,
      (False, ): FactorType.ADD_C0,
      (True, ): FactorType.ADD_C1,
      (False, False): FactorType.ADD_C00,
      (False, True): FactorType.ADD_C01,
      (True, False): FactorType.ADD_C10,
      (True, True): FactorType.ADD_C11
    }

    inputs = [x for x in [a, b, carry] if x.is_rv]
    fixed = [x for x in [a, b, carry] if not x.is_rv]

    # TODO - Need to make a factor for the carry?
    carry_out_is_rv = len(inputs) > 0
    if len(inputs) == 1 and fixed[0].val == False and fixed[1].val == False:
      carry_out_is_rv = False # The single unknown input cannot cause carry_out to be 1
    carry_out = Bit(carry_out_val, carry_out_is_rv)

    if len(inputs) > 0:
      key = tuple(sorted([x.val for x in fixed]))
      f_type = f_types[key]
      Bit.factors.append(Factor(f_type, result, inputs))

    return result, carry


def saveFactors(filename):
  with open(filename, 'w') as f:
    for factor in Bit.factors:
      f.write(str(factor) + '\n')
