#!/usr/bin/python

import collections
import subprocess


class Stat(object):
  def __init__(self):
    self.commits = 0
    self.add_lines = 0
    self.remove_lines = 0


def main():
  log = subprocess.check_output(
      ['git', 'log', '--pretty=format:--- %h %ae', '--numstat', '--no-merges'])

  stats = collections.defaultdict(Stat)

  for entry in log.split('---'):
    lines = entry.strip().splitlines()
    if not lines:
      continue
    hash, mail = lines.pop(0).split()
    stat = stats[mail]
    stat.commits += 1
    for line in lines:
      add_lines, remove_lines, path = line.split(None, 2)
      stat.add_lines += int(add_lines)
      stat.remove_lines += int(remove_lines)

  for mail, stat in sorted(stats.items()):
    print mail, stat.commits, stat.add_lines, stat.remove_lines


if __name__ == '__main__':
  main()
