import os
import torch
import random
import numpy as np
from torch.utils.tensorboard import SummaryWriter

import utils
from opts import PreimageOpts
from models.hashsat import HashSAT
from loss_func import PreimageLoss
from generator import ProblemGenerator
from logic.cnf import CNF
from logic.mis import MaximumIndependentSet


if __name__ == '__main__':
    opts = PreimageOpts()
    random.seed(opts.seed)
    np.random.seed(opts.seed)
    torch.manual_seed(opts.seed)

    logdir = opts.logdir
    exp_name = utils.get_experiment_name(logdir)
    exp_path = os.path.join(logdir, exp_name)
    writer = SummaryWriter(exp_path)

    # writer.add_graph(model)  TODO

    train_seed = 1
    val_seed = 100
    num_inputs = 8
    num_outputs = 8
    num_gates = 8
    num_train_samples = 100
    num_val_samples = 100
    num_test_samples = 100

    gen = ProblemGenerator()
    problem = gen.create_problem(num_inputs, num_outputs, num_gates)
    bits, observed = problem.random_bits()
    cnf = CNF(problem)
    cnf = cnf.simplify(observed)
    mis = MaximumIndependentSet(cnf)
    label = mis.cnf_to_mis_solution(bits)

    # problem = Problem.from_file('./samples/md5_d1_sym.txt')
    # problem = Problem.from_file('./samples/sha256_d8_sym.txt')

    print(f'Problem has num_vars={problem.num_vars}, num_gates={problem.num_gates}')
    print(f'Graph has nodes={mis.num_nodes}, edges={mis.num_edges}')

    model = HashSAT(num_layers=opts.T,
                    hidden_size=opts.d,
                    num_solutions=opts.d)

    loss_fn = PreimageLoss(mis)
    optimizer, scheduler = utils.get_optimizer(opts, model)

    for epoch in range(opts.max_epochs):
        train_rng = np.random.default_rng(train_seed)
        model.train()
        
        cum_loss = 0.0
        count = 0

        for train_idx in range(num_train_samples):
            pred = model(mis.g)

            loss = loss_fn.bce(pred, label)
            cum_loss += loss.item()
            count += 1

            overall_idx = epoch * num_train_samples + train_idx
            writer.add_scalar('train_loss', loss.item(), overall_idx)

            if train_idx % 100 == 0:
                mean_loss = cum_loss / count
                print(f'Train - epoch {epoch} - sample {train_idx} - loss {mean_loss}')
                cum_loss = 0.0
                count = 0

            optimizer.zero_grad()
            loss.backward()
            if opts.grad_clip > 0:
                torch.nn.utils.clip_grad_norm_(model.parameters(), opts.grad_clip)
            optimizer.step()

        val_rng = np.random.default_rng(val_seed)
        model.eval()

        with torch.no_grad():
            num_solved = 0
            for val_idx in range(num_val_samples):
                pred = model(mis.g)
                if loss_fn.get_solution(pred) is not None:
                    num_solved += 1
            print(f'Val - epoch {epoch} - solved {num_solved}/{num_val_samples}')
            writer.add_scalar('val_acc', num_solved / num_val_samples, epoch)

        scheduler.step()  # End of epoch, step the learning rate scheduler
