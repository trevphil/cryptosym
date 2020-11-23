# -*- coding: utf-8 -*-

class Factor(object):
    def __init__(self, data_string):
        parts = data_string.strip().split(';')
        self.factor_type = parts[0]
        self.output_rv = int(parts[1])
        self.input_rvs = []
        for p in parts[2:]:
            self.input_rvs.append(int(p))
        if self.factor_type == 'AND' and len(self.input_rvs) < 2:
            print('Warning: AND factors has %d inputs' % len(self.input_rvs))
        self.referenced_rvs = set(self.input_rvs + [self.output_rv])
