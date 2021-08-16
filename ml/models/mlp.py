import torch


class MLP(torch.nn.Module):
    def __init__(self, dim_in, dims_out):
        super().__init__()
        layers = []
        d = [dim_in] + dims_out
        n = len(d)
        for i in range(n - 1):
            layers.append(torch.nn.Linear(d[i], d[i + 1], bias=True))
            if i < n - 2:
                layers.append(torch.nn.ReLU())
        self.n = n
        self.f = torch.nn.Sequential(*layers)

    def forward(self, x):
        return self.f(x)
