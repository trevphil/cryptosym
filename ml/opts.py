class PreimageOpts(object):
    def __init__(self):
        self.seed = 1
        self.batch_size = 32
        self.max_epochs = 200
        self.train_dataset = "graph_files/train"
        self.val_dataset = "graph_files/val"
        self.test_dataset = "graph_files/test"
        self.d = 32
        self.T = 20
        self.lr_start = 0.001
        self.l2_penalty = 0
        self.lr_decay = 1.0
        self.lr_decay_every_nth = 16
        self.logdir = "log"
        self.grad_clip = 0
        self.sat_loss_weight = 1.0
        self.color_loss_weight = 1.0
        self.use_swa = False
