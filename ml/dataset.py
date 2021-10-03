import os
import dgl
import torch
from pathlib import Path
from dgl.data.utils import load_graphs
from torch.utils.data import DataLoader, Dataset
import pytorch_lightning as pl
from typing import Tuple

from opts import PreimageOpts


class GraphDataset(Dataset):
    def __init__(self, dset_path: Path):
        assert isinstance(dset_path, Path), "Expected Path object"
        self.dset_path = dset_path
        self.graph_files = [f for f in dset_path.iterdir() if f.is_file()]

    def __len__(self) -> int:
        return len(self.graph_files)

    def __getitem__(self, index: int) -> Tuple[dgl.DGLGraph, bool]:
        assert 0 <= index < len(self), f"Index OOB: {index}"
        graph_file = self.graph_files[index]
        graph_list, _ = load_graphs(str(graph_file))
        assert len(graph_list) == 1, "Expected to load 1 graph"
        is_sat = "sat" in graph_file.stem.split("_")
        return graph_list[0], is_sat


class PLDatasetWrapper(pl.LightningDataModule):
    def __init__(self, opts: PreimageOpts):
        super().__init__()
        self.batch_size = opts.batch_size
        self.train_dataset = GraphDataset(Path(opts.train_dataset))
        self.val_dataset = GraphDataset(Path(opts.val_dataset))
        self.test_dataset = GraphDataset(Path(opts.test_dataset))

    def train_dataloader(self) -> DataLoader:
        return DataLoader(
            self.train_dataset,
            batch_size=self.batch_size,
            collate_fn=PLDatasetWrapper.collate_fn,
            num_workers=os.cpu_count(),
            pin_memory=True,
            shuffle=True,
        )

    def val_dataloader(self) -> DataLoader:
        return DataLoader(
            self.val_dataset,
            batch_size=self.batch_size,
            collate_fn=PLDatasetWrapper.collate_fn,
            num_workers=os.cpu_count(),
            pin_memory=True,
            shuffle=False,
        )

    def test_dataloader(self) -> DataLoader:
        return DataLoader(
            self.test_dataset,
            batch_size=self.batch_size,
            collate_fn=PLDatasetWrapper.collate_fn,
            num_workers=os.cpu_count(),
            pin_memory=True,
            shuffle=False,
        )

    @staticmethod
    def collate_fn(graphs_labels) -> Tuple[dgl.DGLGraph, torch.Tensor]:
        graphs, labels = zip(*graphs_labels)
        labels = torch.tensor(labels, dtype=torch.float32)
        return dgl.batch(graphs), labels
