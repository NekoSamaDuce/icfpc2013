"""
Classifier for lightning division plan A.

Example:
./python.sh -m util.lightning_classifier --problemset_file=data/myproblems__do_not_try_before_we_get_ready.tsv --max_cluster_size=20
"""

import os
import subprocess
import sys

import gflags

FLAGS = gflags.FLAGS

SOLVER_DIR = os.path.join(os.path.dirname(__file__), '../solver')

gflags.DEFINE_string(
    'cluster_solver', os.path.join(SOLVER_DIR, 'cluster_main'),
    'Path to cluster solver binary.')
gflags.MarkFlagAsRequired('cluster_solver')

gflags.DEFINE_string(
    'problemset_file', None,
    'Path to the problemset TSV file.')
gflags.MarkFlagAsRequired('problemset_file')

gflags.DEFINE_integer(
    'max_cluster_size', None,
    'Maximum size of a cluster allowed to proceed before starting to solve '
    'a problem. Specify 0 for no threshold.')
gflags.MarkFlagAsRequired('max_cluster_size')


def RunClusterSolver(problem):
  cluster_solver_output = subprocess.check_output(
      [FLAGS.cluster_solver,
       '--size=%d' % problem.size,
       '--operators=%s' % ','.join(problem.operators)])
  assert cluster_solver_output.startswith('argument: ')
  lines = cluster_solver_output.splitlines()
  arguments = [int(s, 0) for s in lines.pop(0).split()[1].split(',')]
  clusters = []
  for line in lines:
    if line.startswith('expected:'):
      expected = [int(s, 0) for s in line.split()[1].split(',')]
      clusters.append((expected, []))
    else:
      clusters[-1][1].append(line)
  return (arguments, clusters)


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


def main():
  sys.argv = FLAGS(sys.argv)

  problems = ReadProblemset()

  for problem in problems:
    arguments, clusters = RunClusterSolver(problem)
    max_cluster_size = max([len(programs) for expected, programs in clusters])
    line = problem.ToProblemLine()
    if max_cluster_size <= FLAGS.max_cluster_size:
      print line
    else:
      print '#', line


if __name__ == '__main__':
  main()
