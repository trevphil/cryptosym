# -*- coding: utf-8 -*-

import os
import h5py
from torch.utils.data import Dataset

class HashReversalDataset(Dataset):
    def __init__(self, purpose, dataset_dir):
        assert purpose in ['train', 'val', 'test']
        self.purpose = purpose

        dset_path = os.path.join(dataset_dir, '%s.hdf5' % self.purpose)
        dset_path = os.path.abspath(dset_path)
        with h5py.File(dset_path, 'r') as hdf5_file:
            self.bits = hdf5_file['.']['bits'].value
            self.target = hdf5_file['.']['target'].value
        assert len(self.bits) == len(self.target)

    def __len__(self):
        return len(self.bits)

    def __getitem__(self, idx):
        return self.bits[idx], self.target[idx]
