# -*- coding: utf-8 -*-
import argparse
import multiprocessing
from os import path
from time import localtime, strftime
from pathlib import Path

from utils.log import getLogger


class Config(object):

  def __init__(self):
    parser = argparse.ArgumentParser(description='SHA256 preimage attacks')
    parser.add_argument('dataset', type=str, help='Path to the CSV dataset file')
    parser.add_argument('--num-workers', type=int, default=multiprocessing.cpu_count(),
                        help='Number of CPU cores to use, to speed things up')
    parser.add_argument('--visualize', action='store_true', default=False,
                        help='Visualize plots/graphs and save them to disk in the background')

    args = parser.parse_args()

    self.dataset = path.abspath(args.dataset)
    self.num_workers = args.num_workers
    self.visualize = args.visualize

    hash_algo = self.dataset.split('/')[-1].split('-')[0]
    self.experiment_dir = path.join(path.abspath('./experiments'),
                                    hash_algo + '_' + strftime('%Y-%m-%d-%H-%M-%S', localtime()))
    self.log_dir = path.join(self.experiment_dir, 'log')

    Path(self.experiment_dir).mkdir(parents=True, exist_ok=False)
    Path(self.log_dir).mkdir(parents=True, exist_ok=False)


  def logConfig(self):
    msg = 'Logging command line arguments:\n'
    for arg in self.__dict__:
      msg += '{} --> {}\n'.format(arg, getattr(self, arg))
    logger = getLogger('config')
    logger.info(msg)
