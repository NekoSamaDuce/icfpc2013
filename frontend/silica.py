"""Silica: Solver frontend v2.

Example:
./python.sh -m frontend.silica --mode=train --size=3 --operators=
./python.sh -m frontend.silica --mode=serious --problemset_file=data/train_small.tsv --problem_id=xop6dUzEtUBAprA0DGVwcAvB
"""

import logging
import os
import subprocess
import sys
import time

import gflags

from frontend import frontend_util
from util import api
from util import stdlog

FLAGS = gflags.FLAGS

SOLVER_DIR = os.path.join(os.path.dirname(__file__), '../solver')

gflags.DEFINE_string(
    'cluster_solver', os.path.join(SOLVER_DIR, 'cluster_main'),
    'Path to cluster solver binary.')
gflags.MarkFlagAsRequired('cluster_solver')

# gflags.DEFINE_string(
#     'batch_evaluate_solver', os.path.join(SOLVER_DIR, 'batch_evaluate'),
#     'Path to batch evaluate solver binary.')
# gflags.MarkFlagAsRequired('batch_evaluate_solver')


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


def BruteForceGuess(problem, programs):
  for program in programs:
    logging.info('=== %s', program)
    try:
      example = api.Guess(problem.id, program)
      logging.info('rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x',
                   example.argument, example.expected, example.actual)
    except api.Solved:
      logging.info('')
      logging.info(u'*\u30fb\u309c\uff9f\uff65*:.\uff61. '
                   u'SOLVED!'
                   u' .\uff61.:*\uff65\u309c\uff9f\uff65*:')
      logging.info('')
      break
  else:
    logging.error('************************************')
    logging.error('NO PROGRAM WAS ACCEPTED. DEAD END...')
    logging.error('************************************')
    sys.exit(1)


def main():
  sys.argv = FLAGS(sys.argv)
  stdlog.setup()

  # Solver existence checks
  assert os.path.exists(FLAGS.cluster_solver)
  # assert os.path.exists(FLAGS.batch_evaluate_solver)

  problem = frontend_util.GetProblemByFlags()

  arguments, clusters = RunClusterSolver(problem)

  cluster_sizes_decreasing = sorted(
      [len(programs) for _, programs in clusters], reverse=True)
  logging.info('Candidate programs: %d', sum(cluster_sizes_decreasing))
  logging.info('Candidate clusters: %d', len(clusters))
  logging.info('Cluster sizes: %s', ', '.join(map(str, cluster_sizes_decreasing)))

  logging.info('Issueing /eval...')
  outputs = api.Eval(problem.id, arguments)

  clusters_map = dict([(tuple(e), c) for e, c in clusters])
  programs = clusters_map.get(tuple(outputs), [])

  #logging.info('%r -> %r', arguments, outputs)
  logging.info('Selected a cluster with population=%d', len(programs))

  BruteForceGuess(problem, programs)


if __name__ == '__main__':
  main()
