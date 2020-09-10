# -*- coding: utf-8 -*-

from copy import deepcopy


class Controller(object):
    def __init__(self):
        self.max_num_better_results = 10
        self.max_num_epochs = 50
        self.states = []

    def __iter__(self):
        self.epoch = 0
        return self

    def reset(self):
        self.states = []

    def __next__(self):
        convergence_reached = False

        if len(self.states) != 0:
            curr_loss = self.states[-1]['loss']
            n_better_states = len([state for state in self.states[-self.max_num_better_results:]
                                  if state['loss'] <= curr_loss])
            convergence_reached = n_better_states >= self.max_num_better_results

        max_epoch_reached = (self.epoch >= self.max_num_epochs)

        if max_epoch_reached:
            print('Training finished because max # epochs (%d) reached' % self.max_num_epochs)
        elif convergence_reached:
            print('Training stopped because convergence reached.')

        if max_epoch_reached or convergence_reached:
            raise StopIteration

        self.epoch += 1
        return self.epoch - 1

    def add_state(self, epoch, loss, model_dict):
        state = {
            'epoch': epoch,
            'loss': float(loss),
            'model_dict': deepcopy(model_dict)
        }
        self.states.append(state)
        self.discard_model_dicts()

    def discard_model_dicts(self):
        sorted_states = sorted(self.states, key=lambda k: k['loss'])
        for state in sorted_states[1:]:
            state.update({'model_dict': {}})

    def get_best_state(self):
        sorted_states = sorted(self.states, key=lambda k: k['loss'])
        return sorted_states[0]
