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


class PLModelWrapper(pl.LightningModule):
    def __init__(self, opts):
        super().__init__()
        self.opts = opts
        self.model = HashSAT(num_layers=opts.T, hidden_size=opts.d)
        self.loss_fn = PreimageLoss()
        self.save_hyperparameters()

    def forward(self, g):
        return self.model(g)

    def configure_optimizers(self):
        optim, optim_sched = utils.get_optimizer(self.opts, self.model)
        return {"optimizer": optim, "lr_scheduler": {"scheduler": optim_sched}}

    def training_step(self, batch, batch_idx):
        batched_graph, sat_labels = batch
        pred_dict = self(batched_graph)
        with batched_graph.local_scope():
            loss_dict = self.loss_fn(batched_graph, pred_dict, sat_labels)

        gs = self.global_step
        bs = float(batched_graph.batch_size)

        loss = loss_dict["loss"]
        sat_loss = loss_dict["sat_loss"]
        color_loss = loss_dict["color_loss"]
        self.logger.experiment.add_scalar("train_loss", loss, gs)
        self.logger.experiment.add_scalar("train_sat_loss", sat_loss, gs)
        self.logger.experiment.add_scalar("train_color_loss", color_loss, gs)

        n_correct_classifications = loss_dict["num_correct_classifications"]
        accuracy = n_correct_classifications / bs
        self.logger.experiment.add_scalar("accuracy", accuracy, gs)

        violated_edges = loss_dict["frac_violated_edges"]
        self.logger.experiment.add_scalar("frac_violated_edges", violated_edges, gs)

        n_solved = loss_dict["num_solved"]
        self.logger.experiment.add_scalar("num_solved", n_solved, gs)

        n_node = loss_dict["mean_num_nodes"]
        n_edge = loss_dict["mean_num_edges"]
        self.logger.experiment.add_scalar("num_nodes", n_node, gs)
        self.logger.experiment.add_scalar("num_edges", n_edge, gs)

        return loss

    def validation_step(self, batch, batch_idx):
        batched_graph, sat_labels = batch
        pred_dict = self(batched_graph)
        with batched_graph.local_scope():
            loss_dict = self.loss_fn(batched_graph, pred_dict, sat_labels)

        bs = float(batched_graph.batch_size)

        self.log("val_loss", loss_dict["loss"])
        self.log("val_num_nodes", loss_dict["mean_num_nodes"])
        self.log("val_num_edges", loss_dict["mean_num_edges"])
        self.log("val_num_solved", loss_dict["num_solved"])

        n_correct_classifications = loss_dict["num_correct_classifications"]
        accuracy = n_correct_classifications / bs
        self.log("val_accuracy", accuracy)

        violated_edges = loss_dict["frac_violated_edges"]
        self.log("val_frac_violated_edges", violated_edges)

        return loss_dict["loss"]

    def test_step(self, batch, batch_idx):
        batched_graph, sat_labels = batch
        pred_dict = self(batched_graph)
        with batched_graph.local_scope():
            loss_dict = self.loss_fn(batched_graph, pred_dict, sat_labels)

        bs = float(batched_graph.batch_size)

        self.log("test_loss", loss_dict["loss"])
        self.log("test_num_nodes", loss_dict["mean_num_nodes"])
        self.log("test_num_edges", loss_dict["mean_num_edges"])
        self.log("test_num_solved", loss_dict["num_solved"])

        n_correct_classifications = loss_dict["num_correct_classifications"]
        accuracy = n_correct_classifications / bs
        self.log("test_accuracy", accuracy)

        violated_edges = loss_dict["frac_violated_edges"]
        self.log("test_frac_violated_edges", violated_edges)

        return loss_dict["num_solved"]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--test", type=str, default="", help="Provide path to the model for testing"
    )
    args = parser.parse_args()
    should_test = len(args.test) > 0

    opts = PreimageOpts()
    random.seed(opts.seed)
    np.random.seed(opts.seed)
    torch.manual_seed(opts.seed)

    pl_dataset = PLDatasetWrapper(opts)

    if not should_test:
        logdir = Path(opts.logdir)
        exp_name = utils.get_experiment_name(logdir)
        logger = TensorBoardLogger(str(logdir / exp_name), name="")

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
            stochastic_weight_avg=True,
        )

        trainer.fit(pl_wrapper, pl_dataset)
    else:
        # Test!
        ckpt_path = Path(args.test)
        if not ckpt_path.exists():
            print(f"Model checkpoint does not exist: {ckpt_path}")
            exit()

        pl_wrapper = PLModelWrapper.load_from_checkpoint(str(ckpt_path), opts=opts)

        logdir = Path(opts.logdir)
        exp_name = utils.get_experiment_name(logdir, is_test=True)
        logger = TensorBoardLogger(str(logdir / exp_name), name="")

        trainer = pl.Trainer(accelerator=None, logger=logger)
        trainer.test(datamodule=pl_dataset, model=pl_wrapper)
