import dgl
import torch
import random
import argparse
import numpy as np
from pathlib import Path
import pytorch_lightning as pl
from pytorch_lightning.loggers import TensorBoardLogger
from pytorch_lightning.callbacks import ModelCheckpoint, LearningRateMonitor
from typing import Dict, Any, Tuple

import utils
from opts import PreimageOpts
# from models.gated_graph_conv import HashSAT
from models.encoder_decoder import HashSAT
from loss_func import PreimageLoss
from dataset import PLDatasetWrapper


class PLModelWrapper(pl.LightningModule):
    def __init__(self, options: PreimageOpts):
        super().__init__()
        self.opts = options
        self.model = HashSAT(t_max=options.T, hidden_size=options.d)
        self.loss_fn = PreimageLoss(options)
        self.save_hyperparameters()

    def forward(self, g: dgl.DGLGraph) -> Dict[str, torch.Tensor]:
        return self.model(g)

    def configure_optimizers(self) -> Dict[str, Any]:
        optim = utils.get_optimizer(self.opts, self.model)
        if isinstance(optim, Tuple):
            optimizer, optimizer_scheduler = optim
            return {
                "optimizer": optimizer,
                "lr_scheduler": {"scheduler": optimizer_scheduler},
            }
        return optim

    def training_step(self, batch: Any, batch_idx: int) -> torch.Tensor:
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

        min_conflicts = loss_dict["min_conflicts"]
        self.logger.experiment.add_scalar("min_conflicts", min_conflicts, gs)

        n_solved = loss_dict["num_solved"]
        self.logger.experiment.add_scalar("num_solved", n_solved, gs)

        n_node = loss_dict["mean_num_nodes"]
        n_edge = loss_dict["mean_num_edges"]
        self.logger.experiment.add_scalar("num_nodes", n_node, gs)
        self.logger.experiment.add_scalar("num_edges", n_edge, gs)

        return loss

    def validation_step(self, batch: Any, batch_idx: int) -> torch.Tensor:
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

        min_conflicts = loss_dict["min_conflicts"]
        self.log("val_min_conflicts", min_conflicts)

        return loss_dict["loss"]

    def test_step(self, batch: Any, batch_idx: int) -> torch.Tensor:
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

        min_conflicts = loss_dict["min_conflicts"]
        self.log("test_min_conflicts", min_conflicts)

        return loss_dict["num_solved"]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "experiment_name",
        type=str,
        help="Brief description of experiment",
    )
    parser.add_argument(
        "--test", type=str, default="", help="Provide path to the model for testing"
    )
    args = parser.parse_args()
    should_test = len(args.test) > 0
    exp_name = args.experiment_name

    opts = PreimageOpts()
    logdir = Path(opts.logdir)
    random.seed(opts.seed)
    np.random.seed(opts.seed)
    torch.manual_seed(opts.seed)

    pl_dataset = PLDatasetWrapper(opts)

    if not should_test:
        exp_name = utils.get_experiment_name(exp_name)
        assert not (logdir / exp_name).exists()
        logger = TensorBoardLogger(str(logdir / exp_name), name="")

        pl_wrapper = PLModelWrapper(opts)
        """
        pl_wrapper = PLModelWrapper.load_from_checkpoint(
            "./log/ggc_2021-10-03/version_0/checkpoints/epoch=18-step=11874.ckpt",
            hparams_file="./log/ggc_2021-10-03/version_0/hparams.yaml",
        )
        """

        checkpoint_cb = ModelCheckpoint(monitor="val_loss", save_top_k=1, mode="min")
        lr_monitor = LearningRateMonitor(logging_interval="step")

        trainer = pl.Trainer(
            accelerator=None,
            logger=logger,
            callbacks=[checkpoint_cb, lr_monitor],
            max_epochs=opts.max_epochs,
            gradient_clip_val=opts.grad_clip,
            stochastic_weight_avg=opts.use_swa,
        )

        trainer.fit(pl_wrapper, pl_dataset)
    else:
        # Test!
        ckpt_path = Path(args.test)
        if not ckpt_path.exists():
            raise FileNotFoundError(f"Model checkpoint does not exist: {ckpt_path}")

        pl_wrapper = PLModelWrapper.load_from_checkpoint(str(ckpt_path), opts=opts)

        exp_name = utils.get_experiment_name(exp_name, is_test=True)
        assert not (logdir / exp_name).exists()
        logger = TensorBoardLogger(str(logdir / exp_name), name="")

        trainer = pl.Trainer(accelerator=None, logger=logger)
        trainer.test(datamodule=pl_dataset, model=pl_wrapper)
