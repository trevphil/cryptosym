import torch
from datetime import datetime
from typing import Tuple, Any


def get_optimizer(opts, model) -> Tuple[torch.optim.Optimizer, Any]:
    lr = opts.lr_start
    l2 = opts.l2_penalty
    decay = opts.lr_decay
    step = opts.lr_decay_every_nth
    optimizer = torch.optim.Adam(model.parameters(), lr=lr, weight_decay=l2)
    # scheduler = torch.optim.lr_scheduler.StepLR(optimizer, step_size=step, gamma=decay)
    scheduler = torch.optim.lr_scheduler.CosineAnnealingWarmRestarts(
        optimizer, T_0=1, T_mult=2, eta_min=5e-5
    )
    return optimizer, scheduler


def get_experiment_name(info: str, is_test: bool = False) -> str:
    dt = datetime.now()
    exp_name = f"{info}_{dt.strftime('%Y-%m-%d')}"
    if is_test:
        exp_name = f"{exp_name}_test"
    return exp_name
