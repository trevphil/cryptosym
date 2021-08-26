import os
import torch
import random
import numpy as np
from torch.utils.tensorboard import SummaryWriter

import utils
import visualization
from opts import PreimageOpts
from models.neurosat import NeuroSAT
from loss_func import PreimageLoss
from generator import ProblemGenerator
from problem import Problem

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
    num_inputs = 32
    num_outputs = 16
    num_gates = 24
    num_train_samples = 100
    num_val_samples = 100
    num_test_samples = 100

    gen = ProblemGenerator(opts)
    problem = gen.create_problem(num_inputs, num_outputs, num_gates)
    # problem = Problem.from_files('./samples/md5_d1_sym.txt', './samples/md5_d1_cnf.txt')
    
    print(f'Problem has n={problem.num_vars}, m={problem.num_gates}')
    dag = visualization.get_dag(problem)
    cnf = visualization.get_cnf(problem)

    model = NeuroSAT(opts, problem)
    loss_fn = PreimageLoss(problem)
    optimizer, scheduler = utils.get_optimizer(opts, model)
    
    train_rng = np.random.default_rng(train_seed)
    observed_bits = problem.random_observed(rng=train_rng)

    for epoch in range(opts.max_epochs):
        # train_rng = np.random.default_rng(train_seed)
        model.train()

        for train_idx in range(num_train_samples):
            # observed_bits = problem.random_observed(rng=train_rng)
            predicted_bits = model(observed_bits)
            
            loss, loss_per_gate, loss_per_output = loss_fn.bce(predicted_bits, observed_bits)
            # visualization.plot_loss_opencv(dag, cnf, loss_per_output)

            overall_idx = epoch * num_train_samples + train_idx
            writer.add_scalar('train_loss', loss.item(), overall_idx)
            for gate_type, gate_loss in loss_per_gate.items():
                writer.add_scalar(f'train_loss - {gate_type}', gate_loss.item(), overall_idx)
            if train_idx % 100 == 0:
                print(f'Train - epoch {epoch} - sample {train_idx} - loss {loss}')

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), opts.grad_clip)
            optimizer.step()

        val_rng = np.random.default_rng(val_seed)
        model.eval()

        with torch.no_grad():
            num_solved = 0
            for val_idx in range(num_val_samples):
                # observed_bits = problem.random_observed()
                predicted_bits = torch.round(model(observed_bits)).int()
                loss = loss_fn.sse(predicted_bits, observed_bits)[0].item()
                num_solved += (1 if loss == 0 else 0)
            print(f'Val - epoch {epoch} - solved {num_solved}/{num_val_samples}')
            writer.add_scalar('val_acc', num_solved / num_val_samples, epoch)

        scheduler.step()  # End of epoch, step the learning rate scheduler
