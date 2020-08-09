# -*- coding: utf-8 -*-
#!/usr/bin/python3

from matplotlib import pyplot as plt

SECTIONS = set([
  'probability bit is one for correct predictions',
  'probability bit is one for incorrect predictions',
  'bit accuracies',
  'factor accuracies',
  'bit mean values'
])


def line2nums(line):
  return [float(x) for x in line.split(',') if len(x.strip()) > 0]


def process_section(section, lines):
  if section == 'probability bit is one for correct predictions':
    return line2nums(lines[0])
  if section == 'probability bit is one for incorrect predictions':
    return line2nums(lines[0])
  if section == 'bit accuracies':
    return [float(x.split(',')[1]) for x in lines]
  if section == 'factor accuracies':
    return {x.split(',')[0]: float(x.split(',')[1]) for x in lines}
  if section == 'bit mean values':
    return [float(x.split(',')[1]) for x in lines]
  raise NotImplementedError('Unknown section: "{}"'.format(section))


def load_data(filename):
  data = dict()

  with open(filename, 'r') as f:
    lines = [line.strip() for line in f if len(line.strip()) > 0]

    for section in SECTIONS:
      section_idx = lines.index(section)
      next_section_idx = section_idx + 1
      while next_section_idx < len(lines):
        if lines[next_section_idx] in SECTIONS:
          break
        next_section_idx += 1
      relevant_lines = lines[section_idx + 1:next_section_idx]
      data[section] = process_section(section, relevant_lines)
      print('Processed section "{}"'.format(section))

  return data


if __name__ == '__main__':
  data = load_data('statistics.txt')
  
  print('Factor accuracies:')
  f_accuracies = data['factor accuracies']
  for f_type in sorted(f_accuracies.keys()):
    print('\t{} --> {}'.format(f_type, f_accuracies[f_type]))

  fig, axs = plt.subplots(1, 2, sharey=True, tight_layout=True)
  axs[0].set_title('Correct predictions')
  axs[0].set_xlabel('Prob. hash input bit is 1')
  axs[0].hist(data['probability bit is one for correct predictions'], bins=30)
  axs[1].set_title('Incorrect predictions')
  axs[1].set_xlabel('Prob. hash input bit is 1')
  axs[1].hist(data['probability bit is one for incorrect predictions'], bins=30)
  
  n = len(data['bit accuracies'])
  m = len(data['bit mean values'])
  assert n == m, '{} != {}'.format(n, m)

  fig, axs = plt.subplots(2, 1, sharex=True, tight_layout=True)
  axs[0].set_title('Bit accuracies')
  axs[0].set_xlabel('Bit index')
  axs[0].set_ylabel('Accuracy [0-1]')
  axs[0].scatter(range(n), data['bit accuracies'], s=0.1)
  axs[1].set_title('Bit mean values')
  axs[1].set_xlabel('Bit index')
  axs[1].set_ylabel('Mean value [0-1]')
  axs[1].scatter(range(n), data['bit mean values'], s=0.1)

  plt.show()
