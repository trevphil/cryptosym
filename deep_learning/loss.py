# -*- coding: utf-8 -*-

import os
import csv
import torch
import torch.nn.functional as F
from time import time


class ReverseHashLoss(object):
    def __init__(self, output_dir, tb_writer, factors, obs_rv_set,
                 obs_rv2idx, num_input_bits):
        self.output_dir = output_dir
        self.tb_writer = tb_writer
        self.factors = factors
        self.num_input_bits = num_input_bits
        self.observed_rvs = list(sorted(obs_rv_set))
        self.obs_rv2idx = obs_rv2idx
        self.purpose = None
        self.epoch = None
        self.batch_results = []
        self.epoch_losses = {'train': dict(), 'val': dict(), 'test': dict()}
        self.report_freq = 100

    def new_epoch(self, epoch, purpose):
        self.purpose = purpose
        self.epoch = epoch
        return self

    def __enter__(self):
        self.batch_results = []

    def __exit__(self, exc_type, exc_val, exc_tb):
        epoch_loss, epoch_accuracy = self.compute_epoch_loss()
        self.epoch_losses[self.purpose].update({str(self.epoch): epoch_loss})

        if self.purpose != 'test':
            print('Epoch {} - {} average loss {:.5f}'.format(
                self.epoch, self.purpose, epoch_loss.item()))
        else:
            print('Test loss: {:.5f}'.format(epoch_loss.item()))

        epoch_loss_dict = self.get_loss_dict(epoch_loss)
        epoch_accuracy_dict = self.get_accuracy_dict(epoch_accuracy)
        self.write_epoch_lossfile(self.epoch, epoch_loss_dict, epoch_accuracy_dict)

        title = '%s/Loss' % self.purpose.capitalize()
        self.tb_writer.add_scalar(title, epoch_loss, self.epoch)
        title = '%s/Accuracy' % self.purpose.capitalize()
        self.tb_writer.add_scalars(title, epoch_accuracy_dict, self.epoch)

    def get_epoch_loss(self):
        return self.epoch_losses[self.purpose][str(self.epoch)]

    def get_loss_dict(self, loss):
        loss_dict = {'Loss': loss.item()}
        return loss_dict

    def get_accuracy_dict(self, accuracy):
        acc_dict = {'Accuracy': accuracy.item()}
        return acc_dict

    def write_epoch_lossfile(self, epoch, epoch_loss_dict, epoch_accuracy_dict):
        row_dict = {'Epoch': epoch}
        row_dict.update(epoch_loss_dict)
        row_dict.update(epoch_accuracy_dict)
        fieldnames = row_dict.keys()

        filepath = os.path.join(self.output_dir, '%s_epoch_losses.csv' % self.purpose)
        if not os.path.exists(filepath):
            with open(filepath, 'w', newline='') as fp:
                csv_writer = csv.DictWriter(fp, fieldnames=fieldnames)
                csv_writer.writeheader()
                csv_writer.writerow(row_dict)
        else:
            with open(filepath, 'a', newline='') as fp:
                csv_writer = csv.DictWriter(fp, fieldnames=fieldnames)
                csv_writer.writerow(row_dict)

    def compute_epoch_loss(self):
        n_samples = 0
        total_loss = 0
        total_acc = 0
        for batch_idx, result in enumerate(self.batch_results):
            n_samples += result['batch_size']
            total_loss += result['loss'] * result['batch_size']
            total_acc += torch.clamp(result['accuracy'] * result['batch_size'], min=0)

        n_samples = float(n_samples)
        epoch_loss = total_loss / n_samples
        epoch_accuracy = total_acc / n_samples
        return epoch_loss, epoch_accuracy

    def __call__(self, batch_idx, predicted_input, target_output):
        start = time()
        loss, accuracy = self.loss_function(predicted_input, target_output)
        comp_time = time() - start

        aggregated_loss = torch.mean(loss)
        acc_dict = self.get_accuracy_dict(accuracy)

        result = {
            'loss': aggregated_loss,
            'batch_size': target_output.size(0),
            'accuracy': accuracy
        }
        self.batch_results.append(result)

        purp = self.purpose.capitalize()
        if len(self.batch_results) % self.report_freq == 0 and self.purpose == 'train':
            print('{} epoch {} - batch {} - loss runtime {:.2f} s - loss {:.5f}'.format(
                purp, self.epoch, len(self.batch_results),
                comp_time, aggregated_loss.item()))

        loss_dict = self.get_loss_dict(aggregated_loss)
        prefix = '{}/Epoch{}'.format(purp, str(self.epoch).zfill(2))
        self.tb_writer.add_scalar('%s/Loss' % prefix, aggregated_loss, batch_idx)
        self.tb_writer.add_scalars('%s/Accuracy' % prefix, acc_dict, batch_idx)

        return aggregated_loss, loss, accuracy

    def loss_function(self, predicted_input, target_output):
        # TODO: Add penalty for inconsistency of input->output
        #       for AND and INV gates
        node_val = dict()
        rvs = sorted(self.factors.keys())

        for rv in rvs:
            factor = self.factors[rv]
            ftype = factor.factor_type
            if ftype == 'PRIOR':
                assert rv < self.num_input_bits
                node_val[rv] = predicted_input[:, rv]
            elif ftype == 'INV':
                node_val[rv] = 1.0 - node_val[factor.input_rvs[0]]
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs
                node_val[rv] = node_val[inp1] * node_val[inp2]

        output = torch.zeros((0, ))
        for rv in self.observed_rvs:
            output = torch.cat((output, node_val[rv]))

        loss = F.binary_cross_entropy_with_logits(
            output, target_output, reduction='none')
        accuracy = torch.zeros(1)
        return loss, accuracy