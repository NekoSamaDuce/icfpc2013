"""Asuna the Lightning: Our Hope.

Example:
./python.sh -m frontend.asuna --cardinal_solver=solver/cardinal --mode=train --size=10 --operators= --train_count=3 --train_exclude_fold --detail_log_dir=details
"""

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

BAILOUT_FILE = os.path.join(os.path.dirname(__file__), os.pardir, 'BAILOUT')

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

gflags.DEFINE_string(
    'detail_log_dir', None,
    'Problem details for post-mortem are logged to this directory.')
gflags.MarkFlagAsRequired('detail_log_dir')


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


def Solve(problem, detail):
  print >>detail, problem
  print >>detail, 'flags: --problem_id=%s --size=%d --operators=%s' % (
      problem.id, problem.size, ','.join(problem.operators))
  print >>detail, ''
  detail.flush()

  logging.info('Issueing /eval...')

  arguments = consts.INABA_KEY
  outputs = api.Eval(problem.id, arguments)
  known_io_pairs = zip(arguments, outputs)

  print >>detail, '=== First Eval ==='
  for argument, output in zip(arguments, outputs):
    print >>detail, '0x%016x => 0x%016x' % (argument, output)
  detail.flush()

  cardinal_argument = []
  cardinal_expected = []
  for _ in xrange(FLAGS.initial_arguments):
    i, o = known_io_pairs.pop()
    cardinal_argument.append(i)
    cardinal_expected.append(o)

  while True:
    print >>detail, ''
    print >>detail, '=== Cardinal Run ==='
    print >>detail, 'arguments:', ','.join(['0x%016x' % x for x in cardinal_argument])
    print >>detail, 'expected:', ','.join(['0x%016x' % x for x in cardinal_expected])
    detail.flush()

    program = RunCardinalSolver(problem, cardinal_argument, cardinal_expected)

    logging.info('=== %s', program)
    print >>detail, 'program:', program
    detail.flush()

    try:
      example = api.Guess(problem.id, program)
    except api.Solved:
      logging.info('')
      logging.info(u'*\u30fb\u309c\uff9f\uff65*:.\uff61. '
                   u'SOLVED!'
                   u' .\uff61.:*\uff65\u309c\uff9f\uff65*:')
      logging.info('')
      print >>detail, '=> success.'
      detail.flush()
      return
    if example:
      logging.info('rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x',
                   example.argument, example.expected, example.actual)
      print >>detail, '=> rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x' % (
          example.argument, example.expected, example.actual)
      detail.flush()
      cardinal_argument.append(example.argument)
      cardinal_expected.append(example.expected)
    else:
      logging.info('rejected, but could not get a counterexample.')
      print >>detail, '=> rejected, but could not get a counterexample.'
      detail.flush()
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
    if os.path.exists(BAILOUT_FILE):
      logging.info('')
      logging.info('Bailed out.')
      sys.exit(0)
    logging.info('******** PROBLEM %d/%d: %r ********',
                 index + 1, len(problems), problem)
    logging.info('Flag to recover: --problem_id=%s --size=%d --operators=%s',
                 problem.id, problem.size, ','.join(problem.operators))
    with open(os.path.join(FLAGS.detail_log_dir, '%s.txt' % problem.id), 'w') as detail:
      Solve(problem, detail)


if __name__ == '__main__':
  main()
