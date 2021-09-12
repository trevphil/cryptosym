import os
import torch
from datetime import datetime


def get_optimizer(opts, model):
    lr = opts.lr_start
    l2 = opts.l2_penalty
    dec = opts.lr_decay
    optimizer = torch.optim.Adam(model.parameters(), lr=lr, weight_decay=l2)
    scheduler = torch.optim.lr_scheduler.ExponentialLR(optimizer, dec)
    return (optimizer, scheduler)


def get_experiment_name(experiment_dir):
    numbers = []
    for name in os.listdir(experiment_dir):
        try:
            num = int(name.split("_")[-1])
            numbers.append(num)
        except BaseException:
            continue

    if len(numbers) == 0:
        exp_num = 1
    else:
        exp_num = max(numbers) + 1

    dt = datetime.now()
    return dt.strftime("%Y-%m-%d-%H-%M-%S") + ("_%02d" % exp_num)
