# -*- coding: utf-8 -*-

class LinkedList:
  """
  Simple linked list implementation for our MD5.
  To conform to the 2.5 MD5 module specification, this list doesn't need
  anything else than addition and a traversed catenated representation
  of itself.

  Originally from: https://github.com/narkkil/md5
  """

  def __init__(self, root):
    self.root = root
    self.tail = root

  def toString(self):
    s = ""
    root = self.root

    while (root != None):
      s += root.value
      root = root.next
    return s


  def add(self, node):
    if self.root == None:
      self.root = node
      self.tail = node
    else:
      self.tail.next = node
      self.tail = node


class Node:
  """A simple node for the list"""
  def __init__(self, value, next_node):
    self.value = value
    self.next = next_node
