"""Silica: Solver frontend v2.

Example:
./python.sh -m frontend.silica --cluster_solver=solver/simplify_main --batch_evaluate_solver=solver/batch_evaluate --mode=train --size=3 --operators= --max_cluster_size=20 --counterexample_filter=false
./python.sh -m frontend.silica --cluster_solver=solver/simplify_main --batch_evaluate_solver=solver/batch_evaluate --mode=serious --problemset_file=data/train_small.tsv --problem_id=xop6dUzEtUBAprA0DGVwcAvB --max_cluster_size=20 --counterexample_filter=false
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

gflags.DEFINE_string(
    'cluster_solver', None,
    'Path to cluster solver binary.')
gflags.MarkFlagAsRequired('cluster_solver')

gflags.DEFINE_string(
    'batch_evaluate_solver', None,
    'Path to batch evaluate solver binary.')
gflags.MarkFlagAsRequired('batch_evaluate_solver')

gflags.DEFINE_integer(
    'max_cluster_size', None,
    'Maximum size of a cluster allowed to proceed before starting to solve '
    'a problem. Specify 0 for no threshold.')
gflags.MarkFlagAsRequired('max_cluster_size')

gflags.DEFINE_boolean(
    'counterexample_filter', None,
    'Call batch_evaluate to filter candidates with counterexamples.')
gflags.MarkFlagAsRequired('counterexample_filter')

gflags.DEFINE_string(
    'cache_dir', '',
    'Path to cache dir')


def RunClusterSolver(problem):
  logging.info('Running clusterer...')
  cluster_solver_output = subprocess.check_output(
      [FLAGS.cluster_solver,
       '--size=%d' % problem.size,
       '--operators=%s' % ','.join(problem.operators),
       '--cache_dir=%s' % FLAGS.cache_dir])
  logging.info('Finished.')
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


def BruteForceGuessOrDie(problem, programs):
  while programs:
    program = programs.pop(0)
    logging.info('=== [%d] %s', len(programs) + 1, program)
    try:
      example = api.Guess(problem.id, program)
    except api.Solved:
      logging.info('')
      logging.info(u'*\u30fb\u309c\uff9f\uff65*:.\uff61. '
                   u'SOLVED!'
                   u' .\uff61.:*\uff65\u309c\uff9f\uff65*:')
      logging.info('')
      return
    if example:
      logging.info('rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x',
                   example.argument, example.expected, example.actual)
      if FLAGS.counterexample_filter:
        programs = FilterProgramsWithCounterExample(programs, example)
    else:
      logging.info('rejected, but could not get a counterexample.')
  logging.error('************************************')
  logging.error('NO PROGRAM WAS ACCEPTED. DEAD END...')
  logging.error('************************************')
  sys.exit(1)


def FilterProgramsWithCounterExample(programs, example):
  if not programs:
    return []
  logging.info('calling batch_evaluate to filter candidates.')
  p = subprocess.Popen(
      [FLAGS.batch_evaluate_solver,
       '--argument=%d' % example.argument],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE)
  evaluate_output = p.communicate('\n'.join(programs))[0]
  if p.returncode != 0:
    logging.error('batch_evaluate failed. Continuing without filter anyway...')
    return programs
  new_programs = []
  for program, output in zip(programs, evaluate_output.splitlines()):
    output = int(output, 0)
    if output == example.expected:
      new_programs.append(program)
  return new_programs


def main():
  sys.argv = FLAGS(sys.argv)
  stdlog.setup()

  # Solver existence checks
  assert os.path.exists(FLAGS.cluster_solver)
  assert os.path.exists(FLAGS.batch_evaluate_solver)

  problems = frontend_util.GetProblemsByFlags()

  for index, problem in enumerate(problems):
    logging.info('******** PROBLEM %d/%d: %r ********',
                 index + 1, len(problems), problem)

    arguments, clusters = RunClusterSolver(problem)

    cluster_sizes_decreasing = sorted(
        [len(programs) for _, programs in clusters], reverse=True)
    logging.info('Candidate programs: %d', sum(cluster_sizes_decreasing))
    logging.info('Candidate clusters: %d', len(clusters))
    logging.info('Cluster sizes: %s', ', '.join(map(str, cluster_sizes_decreasing)))

    if FLAGS.max_cluster_size > 0 and cluster_sizes_decreasing[0] > FLAGS.max_cluster_size:
      logging.error('Maximum cluster size was above threshold (%d)', FLAGS.max_cluster_size)
      logging.error('Stop.')
      sys.exit(1)

    logging.info('Issueing /eval...')
    outputs = api.Eval(problem.id, arguments)

    clusters_map = dict([(tuple(e), c) for e, c in clusters])
    programs = clusters_map.get(tuple(outputs), [])

    #logging.info('%r -> %r', arguments, outputs)
    logging.info('Selected a cluster with population=%d', len(programs))

    BruteForceGuessOrDie(problem, programs)


if __name__ == '__main__':
  main()
