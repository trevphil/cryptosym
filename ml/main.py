import os
import torch
import random
import numpy as np
from torch.utils.tensorboard import SummaryWriter

import utils
from opts import PreimageOpts
from models.neurosat import NeuroSAT
from loss_func import PreimageLoss
from generator import ProblemGenerator
from problem import Problem

if __name__ == '__main__':
  opts = PreimageOpts()
  random.seed(opts.seed)
  np.random.seed(opts.seed)
  torch.manual_seed(opts.seed)

  logdir = opts.logdir
  exp_name = utils.get_experiment_name(logdir)
  exp_path = os.path.join(logdir, exp_name)
  writer = SummaryWriter(exp_path)

  model = NeuroSAT(opts)
  loss_fn = PreimageLoss()
  
  # writer.add_graph(model)  TODO

  gen_train = ProblemGenerator(opts)
  gen_val = ProblemGenerator(opts)
  gen_test = ProblemGenerator(opts)

  optimizer, scheduler = utils.get_optimizer(opts, model)

  num_inputs = 64
  num_outputs = 128
  num_gates = 256
  num_train_samples = 1000
  num_val_samples = 100
  num_test_samples = 100

  train_problem = gen_train.create_problem(num_inputs, num_outputs, num_gates)
  val_problem = train_problem
  
  # val_problem = Problem.from_files('./samples/md5_d1_sym.txt', './samples/md5_d1_cnf.txt')
  # train_problem = val_problem

  for epoch in range(opts.max_epochs):
    model.train()
    for train_idx in range(num_train_samples):
      train_problem.randomize_observed()
      # train_problem = gen_train.create_problem(num_inputs, num_outputs, num_gates)

      pred_bits = model(train_problem)
      loss = loss_fn.bce(pred_bits, train_problem)

      writer.add_scalar('training_loss', loss.item(),
                        epoch * num_train_samples + train_idx)
      if train_idx % 100 == 0:
        print(f'Train - epoch {epoch} - sample {train_idx} - loss {loss}')

      optimizer.zero_grad()
      loss.backward()
      optimizer.step()

    model.eval()
    with torch.no_grad():
      num_solved = 0
      for val_idx in range(num_val_samples):
        val_problem.randomize_observed()
        # val_problem = gen_val.create_problem(num_inputs, num_outputs, num_gates)
        pred_bits = model(val_problem)
        loss = loss_fn.mse(torch.round(pred_bits), val_problem).item()
        num_solved += (1 if loss == 0 else 0)
      print(f'Val - epoch {epoch} - solved {num_solved}/{num_val_samples}')
      writer.add_scalar('val_accuracy', num_solved / num_val_samples, epoch)

    scheduler.step()  # End of epoch, step the learning rate scheduler
