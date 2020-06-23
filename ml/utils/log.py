# -*- coding: utf-8 -*-
import logging
from os import path


LOGGER_ROOT = 'hash_reversal'


def initLogging(logdir, level=logging.INFO):
  logging.basicConfig(level=level)
  logger = getLogger('')
  logger.setLevel(logging.INFO)
  fh = logging.FileHandler(path.join(logdir, 'run.log'))
  fmt_str = '{"time": "%(asctime)s", "logger_name": %(name)s", "level":"%(levelname)s", "message": "%(message)s"}'
  formatter = logging.Formatter(fmt_str)
  fh.setFormatter(formatter)
  fh.setLevel(logging.DEBUG)
  logger.addHandler(fh)
  return logger


def getLogger(name=''):
  logger_name = LOGGER_ROOT
  if len(name) != 0:
    logger_name += '.%s' % name
  return logging.getLogger(logger_name)
