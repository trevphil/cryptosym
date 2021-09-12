import os
import dgl
import torch
import random
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


class PLModelWrapper(pl.LightningModule):
    def __init__(self, opts):
        super().__init__()
        self.opts = opts
        self.model = HashSAT(
            num_layers=opts.T, hidden_size=opts.d, num_solutions=opts.d
        )
        self.loss_fn = PreimageLoss()

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
            loss = self.loss_fn.bce(batched_graph, labels)

        self.logger.experiment.add_scalar("train_loss", loss, self.global_step)

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
            mis_size = self.loss_fn.mis_size_ratio(mis_samples, batched_graph)

        self.logger.experiment.add_scalar("val_mis_size", mis_size, self.global_step)
        self.log("pl_mis_size", mis_size)

        return mis_size

    def test_step(self, batch, batch_idx):
        mis_samples, batched_graph, labels = batch
        graphs = dgl.unbatch(batched_graph)

        solver = BasicMISSolver(self.model)
        num_solved = 0

        for mis, g in zip(mis_samples, graphs):
            with g.local_scope():
                solution = solver.solve(mis, g, no_model=False)
                mis_size = torch.sum(solution).detach().item()
                if mis_size == mis.expected_num_clauses:
                    num_solved += 1

        self.logger.experiment.add_scalar("test_solved", num_solved, self.global_step)

        return num_solved


if __name__ == "__main__":
    opts = PreimageOpts()
    random.seed(opts.seed)
    np.random.seed(opts.seed)
    torch.manual_seed(opts.seed)

    logdir = opts.logdir
    exp_name = utils.get_experiment_name(logdir)
    exp_path = os.path.join(logdir, exp_name)
    logger = TensorBoardLogger(exp_path, name="")

    pl_wrapper = PLModelWrapper(opts)
    pl_dataset = PLDatasetWrapper(opts)

    checkpoint_cb = ModelCheckpoint(monitor="pl_mis_size", save_top_k=1, mode="max")
    lr_monitor = LearningRateMonitor(logging_interval="step")

    trainer = pl.Trainer(
        accelerator=None,
        logger=logger,
        callbacks=[checkpoint_cb, lr_monitor],
        max_epochs=opts.max_epochs,
        gradient_clip_val=opts.grad_clip,
    )

    # trainer.fit(pl_wrapper, pl_dataset)

    ckpt_path = (
        "./log/2021-09-09-20-28-47_01/version_0/checkpoints/epoch=3-step=783.ckpt"
    )
    pl_wrapper = PLModelWrapper.load_from_checkpoint(ckpt_path, opts=opts)
    trainer.test(datamodule=pl_dataset, model=pl_wrapper)
