# -*- coding: utf-8 -*-
import argparse
from os import path
from time import localtime, strftime
from pathlib import Path

from utils.log import getLogger


class Config(object):

  def __init__(self):
    parser = argparse.ArgumentParser(description='SHA256 preimage attacks')
    parser.add_argument('dataset', type=str, help='Path to the CSV dataset file')
    parser.add_argument('graph', type=str, help='Path to CSV file with BN adjacency matrix')

    args = parser.parse_args()

    self.dataset = path.abspath(args.dataset)
    self.graph = path.abspath(args.graph)
    self.visualize = True

    self.hash_algo = self.dataset.split('/')[-1].split('-')[0]
    self.experiment_dir = path.join(path.abspath('./experiments'),
                                    self.hash_algo + '_' + strftime('%Y-%m-%d-%H-%M-%S', localtime()))
    self.log_dir = path.join(self.experiment_dir, 'log')

    self.data_dir = path.abspath('./data')
    self.fcg_data_file = path.join(self.data_dir, 'bn_fully_connected.yaml')

    Path(self.experiment_dir).mkdir(parents=True, exist_ok=False)
    Path(self.log_dir).mkdir(parents=True, exist_ok=False)


  def logConfig(self):
    msg = 'Logging command line arguments:\n'
    for arg in self.__dict__:
      msg += '{} --> {}\n'.format(arg, getattr(self, arg))
    logger = getLogger('config')
    logger.info(msg)
