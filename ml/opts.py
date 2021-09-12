import torch


class PreimageOpts(object):
    def __init__(self):
        self.seed = 1
        self.batch_size = 32
        self.max_epochs = 200
        self.train_dataset = "graph_dataset/train"
        self.val_dataset = "graph_dataset/val"
        self.test_dataset = "graph_dataset/test"
        self.d = 32
        self.T = 20
        self.lr_start = 0.001
        self.lr_decay = 1.00
        self.l2_penalty = 5e-4
        self.logdir = "log"
        self.grad_clip = 0.0
