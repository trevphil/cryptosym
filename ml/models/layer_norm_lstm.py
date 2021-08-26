import torch

class LayerNormLSTMCell(torch.nn.Module):
    def __init__(self, input_size, hidden_size):
        super().__init__()
        self.input_size = input_size
        self.hidden_size = hidden_size
        self.fiou_linear = torch.nn.Linear(
            input_size + hidden_size, hidden_size * 4, bias=False)
        self.fiou_ln_layers = torch.nn.ModuleList(
            torch.nn.LayerNorm(hidden_size) for _ in range(4))
        self.cell_ln = torch.nn.LayerNorm(hidden_size)

    def forward(self, x, state):
        hidden_tensor, cell_tensor = state

        fiou_linear = self.fiou_linear(
            torch.cat([x, hidden_tensor], dim=1))
        fiou_linear_tensors = fiou_linear.split(self.hidden_size, dim=1)

        # if self.layer_norm_enabled:
        fiou_linear_tensors = tuple(ln(tensor) for ln, tensor in zip(
            self.fiou_ln_layers, fiou_linear_tensors))

        f, i, o = tuple(torch.sigmoid(tensor)
                        for tensor in fiou_linear_tensors[:3])
        u = torch.tanh(fiou_linear_tensors[3])

        new_cell = self.cell_ln(i * u + (f * cell_tensor))
        new_h = o * torch.tanh(new_cell)

        return new_h, new_cell
