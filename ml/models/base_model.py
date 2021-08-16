import torch


class BaseModel(torch.nn.Module):
    def __init__(self, opts):
        super().__init__()
        self.opts = opts

    @property
    def name(self):
        raise NotImplementedError('Provide model name in subclasses')

    @property
    def num_parameters(self):
        return sum(p.numel() for p in self.parameters())
