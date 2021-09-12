import os
import dgl
import torch
from pathlib import Path
from dgl.data.utils import load_graphs
from torch.utils.data import DataLoader, Dataset
import pytorch_lightning as pl

from logic.mis import MaximumIndependentSet


class MISDataset(Dataset):
    def __init__(self, dset_path):
        assert isinstance(dset_path, Path), "Expected Path object"
        self.dset_path = dset_path
        self.graph_files = [f for f in dset_path.iterdir() if f.is_file()]

    def __len__(self):
        return len(self.graph_files)

    def __getitem__(self, index):
        assert 0 <= index < len(self), f"Index OOB: {index}"
        graph_list, _ = load_graphs(str(self.graph_files[index]))
        assert len(graph_list) == 1, "Expected to load 1 graph"
        return MaximumIndependentSet(g=graph_list[0])


class PLDatasetWrapper(pl.LightningDataModule):
    def __init__(self, opts):
        super().__init__()
        self.batch_size = opts.batch_size
        self.train_dataset = MISDataset(Path(opts.train_dataset))
        self.val_dataset = MISDataset(Path(opts.val_dataset))
        self.test_dataset = MISDataset(Path(opts.test_dataset))

    def train_dataloader(self):
        return DataLoader(
            self.train_dataset,
            batch_size=self.batch_size,
            collate_fn=self.collate_fn,
            num_workers=os.cpu_count(),
            pin_memory=True,
        )

    def val_dataloader(self):
        return DataLoader(
            self.val_dataset,
            batch_size=self.batch_size,
            collate_fn=self.collate_fn,
            num_workers=os.cpu_count(),
            pin_memory=True,
        )

    def test_dataloader(self):
        return DataLoader(
            self.test_dataset,
            batch_size=self.batch_size,
            collate_fn=self.collate_fn,
            num_workers=os.cpu_count(),
            pin_memory=True,
        )

    def collate_fn(self, mis_samples):
        labels = []
        graphs = []
        for mis in mis_samples:
            label = mis.g.ndata["label"]
            assert label.size(1) > 0, "Empty label (no SAT solution)!"
            labels.append(label)
            mis.g.ndata["label"] = torch.zeros(label.size(0), 0)
            graphs.append(mis.g)
        return mis_samples, dgl.batch(graphs), labels
