"""
Concurrent preclusterer.

Example:
./python.sh -m util.preclusterer --cluster_solver=solver/simplify_main --problemset_file=data/myproblems__do_not_try_before_we_get_ready.tsv --threads=12 --time_limit_sec=300 --memory_limit_kb=10000000
"""

import logging
import os
import Queue
import subprocess
import sys
import threading

import gflags
from util import stdlog

FLAGS = gflags.FLAGS

ROOT_DIR = os.path.join(os.path.dirname(__file__), os.pardir)

gflags.DEFINE_string(
    'cluster_solver', None,
    'Path to cluster solver binary.')
gflags.MarkFlagAsRequired('cluster_solver')

gflags.DEFINE_string(
    'problemset_file', None,
    'Path to the problemset TSV file.')
gflags.MarkFlagAsRequired('problemset_file')

gflags.DEFINE_integer(
    'threads', None,
    'Number of threads.')
gflags.MarkFlagAsRequired('threads')

gflags.DEFINE_integer(
    'time_limit_sec', None,
    'Time limit per problem in seconds.')
gflags.MarkFlagAsRequired('time_limit_sec')

gflags.DEFINE_integer(
    'memory_limit_kb', None,
    'Memory limit per problem in KB.')
gflags.MarkFlagAsRequired('memory_limit_kb')


class Problem(object):
  def __init__(self, id, size, operators, answer=None, solved=None, time_left=None):
    self.id = id
    self.size = size
    self.operators = operators
    self.answer = answer
    self.solved = solved
    self.time_left = time_left

  def AsDict(self):
    data = {
      'id': self.id,
      'size': self.size,
      'operators': self.operators,
      }
    if self.answer is not None:
      data['answer'] = self.answer
    if self.solved is not None:
      data['solved'] = self.solved
    if self.time_left is not None:
      data['timeLeft'] = self.time_left
    return data

  def ToProblemLine(self):
    return '\t'.join([self.id, str(self.size), ','.join(self.operators)])

  def __repr__(self):
    v = ['Problem']
    v.append('id=%r' % self.id)
    v.append('size=%r' % self.size)
    v.append('operators=%r' % self.operators)
    if self.answer is not None:
      v.append('answer=%r' % self.answer)
    if self.solved is not None:
      v.append('solved=%r' % self.solved)
    if self.time_left is not None:
      v.append('time_left=%r' % self.time_left)
    return '<%s>' % ' '.join(v)


def ReadProblemset():
  problems = []
  with open(FLAGS.problemset_file) as f:
    for line in f:
      if line.startswith('#'):
        continue
      id, size, operators = line.rstrip('\n').split('\t')[:3]
      size = int(size)
      operators = operators.split(',')
      problems.append(Problem(id, size, operators))
  return problems


def Worker(q):
  while True:
    problem = q.get()
    with open(os.devnull, 'w') as null:
      p = subprocess.Popen(
          ['/bin/bash', '-c',
           'ulimit -t %d -v %d; eval "$@" | %s' % (
                FLAGS.time_limit_sec, FLAGS.memory_limit_kb,
                '%s/python.sh -m util.supercacher' % ROOT_DIR),
           'go',
           FLAGS.cluster_solver,
           '--size=%d' % problem.size,
           '--operators=%s' % ','.join(problem.operators)],
          stdin=subprocess.PIPE,
          stdout=subprocess.PIPE,
          stderr=null)
    line = problem.ToProblemLine().replace('\t', ' ')
    logging.info('start: %s', line)
    output = p.communicate(None)[0]
    if output.strip() != 'ok':
      logging.info('FAIL: %s', line)
    else:
      logging.info('SUCCESS: %s => %s', line, ','.join(output.split()))
    q.task_done()


def main():
  sys.argv = FLAGS(sys.argv)
  stdlog.setup()

  problems = ReadProblemset()

  q = Queue.Queue()

  for problem in problems:
    q.put(problem)

  for _ in xrange(FLAGS.threads):
    th = threading.Thread(target=Worker, args=(q,))
    th.daemon = True
    th.start()

  q.join()


if __name__ == '__main__':
  main()
