class PreimageOpts(object):
    def __init__(self):
        self.seed = 1
        self.batch_size = 16
        self.max_epochs = 200
        self.train_dataset = "graph_files/train"
        self.val_dataset = "graph_files/val"
        self.test_dataset = "graph_files/test"
        self.d = 32
        self.T = 20
        self.lr_start = 0.0005
        self.lr_decay = 1.00
        self.l2_penalty = 0
        self.logdir = "log"
        self.grad_clip = 0
