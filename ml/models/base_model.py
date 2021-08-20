import torch


class BaseModel(torch.nn.Module):
    def __init__(self, opts, problem):
        super().__init__()
        self.opts = opts
        self.problem = problem

    @property
    def name(self):
        raise NotImplementedError('Provide model name in subclass')

    @property
    def num_parameters(self):
        return sum(p.numel() for p in self.parameters())
