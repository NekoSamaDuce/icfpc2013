"""Asuna the Lightning: Our Hope."""

import logging
import os
import subprocess
import sys
import time

import gflags

from frontend import frontend_util
from util import api
from util import consts
from util import stdlog

FLAGS = gflags.FLAGS

gflags.DEFINE_string(
    'cardinal_solver', None,
    'Path to cardinal solver binary.')
gflags.MarkFlagAsRequired('cardinal_solver')

# gflags.DEFINE_string(
#     'batch_evaluate_solver', None,
#     'Path to batch evaluate solver binary.')
# gflags.MarkFlagAsRequired('batch_evaluate_solver')

gflags.DEFINE_integer(
    'initial_arguments', 3,
    'Number of arguments initially given to cardinal.')


def RunCardinalSolver(problem, argument, expected):
  logging.info('Running Cardinal System with %d arguments...', len(argument))
  output = subprocess.check_output(
      [FLAGS.cardinal_solver,
       '--size=%d' % problem.size,
       '--operators=%s' % ','.join(problem.operators),
       '--argument=%s' % ','.join(map(str, argument)),
       '--expected=%s' % ','.join(map(str, expected)),
       ])
  logging.info('Finished.')
  return output.strip()


def Solve(problem):
  logging.info('Issueing /eval...')

  arguments = consts.INABA_KEY
  outputs = api.Eval(problem.id, arguments)
  known_io_pairs = zip(arguments, outputs)

  cardinal_argument = []
  cardinal_expected = []
  for _ in xrange(FLAGS.initial_arguments):
    i, o = known_io_pairs.pop()
    cardinal_argument.append(i)
    cardinal_expected.append(o)

  while True:
    program = RunCardinalSolver(problem, cardinal_argument, cardinal_expected)
    logging.info('=== %s', program)
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
    else:
      logging.info('rejected, but could not get a counterexample.')
    i, o = known_io_pairs.pop()
    cardinal_argument.append(i)
    cardinal_expected.append(o)


def main():
  sys.argv = FLAGS(sys.argv)
  stdlog.setup()

  # Solver existence checks
  assert os.path.exists(FLAGS.cardinal_solver)
  # assert os.path.exists(FLAGS.batch_evaluate_solver)

  problems = frontend_util.GetProblemsByFlags()

  for index, problem in enumerate(problems):
    logging.info('******** PROBLEM %d/%d: %r ********',
                 index + 1, len(problems), problem)
    Solve(problem)


if __name__ == '__main__':
  main()
