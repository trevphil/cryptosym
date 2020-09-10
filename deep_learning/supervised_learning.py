# -*- coding: utf-8 -*-

import os
import torch
import subprocess
import numpy as np
from torch import optim
from BitVector import BitVector
from collections import defaultdict
from torch.utils.tensorboard import SummaryWriter

from deep_learning.models import ReverseHashModel
from deep_learning.controller import Controller
from deep_learning.loss import ReverseHashLoss


class SupervisedLearning(object):
    def __init__(self, output_dir, config, factors, dataloaders):
        self.output_dir = output_dir
        self.config = config
        self.factors = factors
        self.dataloaders = dataloaders
        self.loss = None
        self.tb_writer = None

        self.n_input = int(self.config['num_input_bits'])
        self.observed_rvs = self.config['observed_rv_indices']
        self.n_observed = len(self.observed_rvs)
        self.obs_rv2idx = {rv: i for i, rv in enumerate(self.observed_rvs)}

        parents_per_rv = defaultdict(lambda: 0)
        for _, factor in factors.items():
            for input_rv in factor.input_rvs:
                parents_per_rv[input_rv] += 1

        self.model = ReverseHashModel(factors, self.observed_rvs, self.obs_rv2idx,
                                      self.n_input, parents_per_rv)
        self.optimizer = optim.Adam(self.model.parameters(), lr=0.0001)
        self.controller = Controller()

    def __enter__(self):
        log_dir = os.path.join(self.output_dir, 'tb_logs')
        self.tb_writer = SummaryWriter(log_dir)

        self.loss = ReverseHashLoss(self.output_dir, self.tb_writer, self.factors,
                                    self.observed_rvs, self.obs_rv2idx, self.n_input)

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.tb_writer.close()

    def forward_pass(self, all_bits):
        model_input = torch.zeros(self.n_observed)
        for i, rv in enumerate(self.observed_rvs):
            model_input[i] = all_bits[rv]
        model_input = torch.reshape(model_input, (1, self.n_observed))
        return self.model(model_input)

    def train(self):
        self.controller.reset()
        for epoch in self.controller:
            self.train_epoch(epoch)
            self.validate_epoch(epoch)

    def train_epoch(self, epoch):
        self.model.train()

        with self.loss.new_epoch(epoch, 'train'):
            for batch_idx, (all_bits, target) in enumerate(self.dataloaders['train']):
                self.optimizer.zero_grad()
                pred = self.forward_pass(all_bits)
                aggregated_loss, _, _ = self.loss(batch_idx, pred, target)
                aggregated_loss.backward()
                self.optimizer.step()

    def validate_epoch(self, epoch):
        self.model.eval()

        with self.loss.new_epoch(epoch, 'val'), torch.no_grad():
            for batch_idx, (all_bits, target) in enumerate(self.dataloaders['val']):
                pred = self.forward_pass(all_bits)
                _ = self.loss(batch_idx, pred, target)

        self.controller.add_state(epoch, self.loss.get_epoch_loss(), self.model.state_dict())

    def test(self):
        self.model.eval()

        with self.loss.new_epoch(0, 'test'), torch.no_grad():
            for batch_idx, (all_bits, target) in enumerate(self.dataloaders['test']):
                pred = self.forward_pass(all_bits)
                _ = self.loss(batch_idx, pred, target)

                if batch_idx < 10:
                    self.verify(batch_idx, pred, all_bits[:self.n_input])

    def save_best_model(self):
        best_dict = self.controller.get_best_state()['model_dict']
        print('Loading the best model state from past epochs...')
        self.model.load_state_dict(best_dict)
        self.save_model(self.model)

    def save_model(self, model):
        model_path = os.path.join(self.output_dir, 'model.pt')
        print('Saving model as: %s' % model_path)
        torch.save(model.state_dict(), str(model_path))

    def verify(self, test_idx, predicted_input, true_input):
        hash_algo = self.config['hash']
        difficulty = self.config['difficulty']

        clipped = predicted_input.clone().detach()
        clipped[clipped > 0.5] = 1
        clipped[clipped <= 0.5] = 0

        true_in = int(BitVector(bitlist=true_input.squeeze()))
        pred_in = int(BitVector(bitlist=clipped.squeeze()))

        fmt = '{:0%dX}' % (self.n_input // 4)
        true_in = fmt.format(true_in).lower()
        pred_in = fmt.format(pred_in).lower()
        print('TEST CASE %d' % test_idx)
        print('Hash input: %s' % true_in)
        print('Pred input: %s' % pred_in)

        cmd = ['python', '-m', 'dataset_generation.generate',
            '--num-input-bits', str(self.n_input),
            '--hash-algo', hash_algo, '--difficulty', str(difficulty),
            '--hash-input']

        true_out = subprocess.run(cmd + [true_in],
                                stdout=subprocess.PIPE).stdout.decode('utf-8')
        pred_out = subprocess.run(cmd + [pred_in],
                                stdout=subprocess.PIPE).stdout.decode('utf-8')

        if true_out == pred_out:
            print('Hashes match: {}'.format(true_out))
        else:
            print('Expected:\n\t{}\nGot:\n\t{}'.format(true_out, pred_out))
