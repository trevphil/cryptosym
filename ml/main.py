import os
import dgl
import torch
import random
import argparse
import numpy as np
from pathlib import Path
import pytorch_lightning as pl
from pytorch_lightning.loggers import TensorBoardLogger
from pytorch_lightning.callbacks import ModelCheckpoint, LearningRateMonitor

import utils
from opts import PreimageOpts
from models.hashsat import HashSAT
from loss_func import PreimageLoss
from dataset import PLDatasetWrapper
from solver.basic_solver import BasicMISSolver
from solver.tree_solver import TreeMISSolver


class PLModelWrapper(pl.LightningModule):
    def __init__(self, opts):
        super().__init__()
        self.opts = opts
        self.model = HashSAT(
            num_layers=opts.T, hidden_size=opts.d, num_solutions=opts.d
        )
        self.loss_fn = PreimageLoss()
        self.register_buffer("solution_weights", torch.zeros(opts.d, dtype=int))
        self.save_hyperparameters()

    def forward(self, g):
        return self.model(g)

    def configure_optimizers(self):
        optim, optim_sched = utils.get_optimizer(self.opts, self.model)
        return {"optimizer": optim, "lr_scheduler": {"scheduler": optim_sched}}

    def training_step(self, batch, batch_idx):
        mis_samples, batched_graph, labels = batch
        pred = self(batched_graph)

        with batched_graph.local_scope():
            batched_graph.ndata["pred"] = pred
            loss, votes = self.loss_fn.bce(batched_graph, labels, self.opts.d)
            self.solution_weights += votes

        self.logger.experiment.add_scalar("train_loss", loss, self.global_step)
        if self.global_step % 20 == 0:
            print(f"solution_weights: {self.solution_weights}")

        batch_size = float(len(mis_samples))
        n_node = sum(mis.g.num_nodes() for mis in mis_samples) / batch_size
        n_edge = sum(mis.g.num_edges() for mis in mis_samples) / batch_size

        self.logger.experiment.add_scalar("num_nodes", n_node, self.global_step)
        self.logger.experiment.add_scalar("num_edges", n_edge, self.global_step)

        return loss

    def validation_step(self, batch, batch_idx):
        mis_samples, batched_graph, labels = batch
        pred = self(batched_graph)

        with batched_graph.local_scope():
            batched_graph.ndata["pred"] = pred
            loss, _ = self.loss_fn.bce(batched_graph, labels, self.opts.d)
            mis_size = self.loss_fn.mis_size_ratio(mis_samples, batched_graph)

        self.logger.experiment.add_scalar("val_mis_size", mis_size, self.global_step)
        self.log("pl_mis_size", mis_size)
        self.log("val_loss", loss)

        return mis_size

    def test_step(self, batch, batch_idx):
        mis_samples, batched_graph, labels = batch
        graphs = dgl.unbatch(batched_graph)

        solver = TreeMISSolver(self.model, self.solution_weights)
        num_solved = 0

        for mis, g in zip(mis_samples, graphs):
            with g.local_scope():
                solution = solver.solve(mis, g, no_model=False)
                if isinstance(solution, torch.Tensor):
                    mis_size = torch.sum(solution).detach().item()
                    if mis_size == mis.expected_num_clauses:
                        num_solved += 1

        self.logger.experiment.add_scalar("test_solved", num_solved, self.global_step)

        return num_solved


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--test", type=str, default='',
                        help="Provide path to the model for testing")
    args = parser.parse_args()
    should_test = len(args.test) > 0

    opts = PreimageOpts()
    random.seed(opts.seed)
    np.random.seed(opts.seed)
    torch.manual_seed(opts.seed)
    
    pl_dataset = PLDatasetWrapper(opts)

    if not should_test:
        logdir = opts.logdir
        exp_name = utils.get_experiment_name(logdir)
        exp_path = os.path.join(logdir, exp_name)
        logger = TensorBoardLogger(exp_path, name="")

        pl_wrapper = PLModelWrapper(opts)

        # checkpoint_cb = ModelCheckpoint(monitor="pl_mis_size", save_top_k=1, mode="max")
        checkpoint_cb = ModelCheckpoint(monitor="val_loss", save_top_k=1, mode="min")
        lr_monitor = LearningRateMonitor(logging_interval="step")

        trainer = pl.Trainer(
            accelerator=None,
            logger=logger,
            callbacks=[checkpoint_cb, lr_monitor],
            max_epochs=opts.max_epochs,
            gradient_clip_val=opts.grad_clip,
        )

        trainer.fit(pl_wrapper, pl_dataset)
    else:
        # Test!
        ckpt_path = args.test
        if not os.path.exists(ckpt_path):
            print(f"Model checkpoint does not exist: '{ckpt_path}'")
            exit()

        pl_wrapper = PLModelWrapper.load_from_checkpoint(ckpt_path, opts=opts)
        assert pl_wrapper.solution_weights.any(), "'solution_weights' is all 0"

        logdir = opts.logdir
        exp_name = utils.get_experiment_name(logdir, is_test=True)
        exp_path = os.path.join(logdir, exp_name)
        logger = TensorBoardLogger(exp_path, name="")

        trainer = pl.Trainer(accelerator=None, logger=logger)
        trainer.test(datamodule=pl_dataset, model=pl_wrapper)
