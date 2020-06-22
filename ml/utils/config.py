# -*- coding: utf-8 -*-
import argparse

from utils.log import getLogger, logArgs


class Config(object):

  def __init__(self):
    parser = argparse.ArgumentParser(description='SHA256 preimage attacks')
    parser.add_argument('--num-workers', type=int, default=1, metavar='num-workers',
                        help='Number of CPU cores to use, to speed things up')
    parser.add_argument('--visualize', action='store_true', default=False,
                        help='Visualize plots/graphs and save them to disk in the background')

    args = parser.parse_args()

    self.logger = getLogger('config')
    self.num_workers = args.num_workers
    self.visualize = args.visualize
    logArgs(self.logger, args)
