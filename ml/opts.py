import torch


class PreimageOpts(object):
    def __init__(self):
        self.seed = 1
        self.dtype = torch.float32
        self.d = 128
        self.T = 10
        self.n_msg_layers = 3
        self.n_vote_layers = 3
        self.lr_start = 1e-4
        self.lr_decay = 0.99
        self.l2_penalty = 1e-9
        self.max_epochs = 50
        self.logdir = 'log'
