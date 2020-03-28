# -*- coding: utf-8 -*-
import numpy as np
from numpy import loadtxt
from sklearn.model_selection import train_test_split

from keras.models import Sequential
from keras.layers import Dense, Conv1D, MaxPooling1D, Flatten
from keras.regularizers import l2

"""
Here is an attempt to train a simple neural network
whose inputs are the 256 bits of a SHA-256 hash, and the
output is single bit from the hash input which was using
to generate the hash.

It fails miserably. Accuracy is almost exactly 50%.
I was playing around with architectures, e.g. LSTM, but to no avail.
"""

# load the dataset
dataset = loadtxt('./data/data.csv', delimiter=',')

# split into input (X) and output (y) variables
X = dataset[:, :256]
X = X.reshape((-1, 256, 1))
y = dataset[:, 256]

# fix random seed for reproducibility
seed = 7
np.random.seed(seed)

# create a train-test split
X_train, X_test, y_train, y_test = \
  train_test_split(X, y, test_size=0.2, random_state=seed)

# define the keras model
R = 0 # 0.0001
model = Sequential()
model.add(Conv1D(32, 8, kernel_regularizer=l2(R)))
model.add(MaxPooling1D(pool_size=(2,)))
model.add(Conv1D(64, 4, kernel_regularizer=l2(R)))
model.add(MaxPooling1D(pool_size=(2,)))
model.add(Dense(8, activation='relu', kernel_regularizer=l2(R)))
model.add(Flatten())
model.add(Dense(1, activation='sigmoid'))

# compile the keras model
model.compile(
  loss='binary_crossentropy',
  optimizer='adam',
  metrics=['accuracy']
)

# fit the keras model on the dataset
model.fit(
  X_train, y_train,
  validation_data=(X_test, y_test),
  epochs=150,
  batch_size=1)

# evaluate the keras model
_, accuracy = model.evaluate(X, y)
print('Accuracy: %.2f' % accuracy * 100)
