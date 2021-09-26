import torch
from pathlib import Path
from datetime import datetime


def get_optimizer(opts, model):
    lr = opts.lr_start
    l2 = opts.l2_penalty
    dec = opts.lr_decay
    optimizer = torch.optim.Adam(model.parameters(), lr=lr, weight_decay=l2)
    scheduler = torch.optim.lr_scheduler.ExponentialLR(optimizer, dec)
    return (optimizer, scheduler)


def get_experiment_name(experiment_dir: Path, is_test: bool = False) -> str:
    numbers = []
    for name in experiment_dir.iterdir():
        try:
            num = int(name.stem.split("_")[-1])
            numbers.append(num)
        except BaseException:
            continue

    if len(numbers) == 0:
        exp_num = 1
    else:
        exp_num = max(numbers) + 1

    dt = datetime.now()
    exp_name = dt.strftime("%Y-%m-%d-%H-%M-%S") + ("_%02d" % exp_num)
    if is_test:
        exp_name = f"test_{exp_name}"
    return exp_name
