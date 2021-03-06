"""Asuna the Lightning: Our Hope.

Example:
./python.sh -m frontend.asuna --cardinal_solver=solver/cardinal --mode=train --size=10 --operators= --train_count=3 --train_exclude_fold --detail_log_dir=details
"""

import logging
import os
import random
import signal
import subprocess
import sys
import threading
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

gflags.DEFINE_boolean(
    'keep_going', False,
    'Keep going even on expiration')

gflags.DEFINE_integer(
    'time_limit_sec', 300,
    'Time limit in seconds')


def RunCardinalSolver(problem, argument, expected,
                      refinement_argument, refinement_expected, detail, timeout_sec):
  random_seed = random.randrange(0, 1000000)
  print >>detail, ''
  print >>detail, '=== Cardinal Run ==='
  print >>detail, 'random_seed:', random_seed
  print >>detail, 'argument:', ','.join(['0x%016x' % x for x in argument])
  print >>detail, 'expected:', ','.join(['0x%016x' % x for x in expected])
  print >>detail, 'refinement_argument:', ','.join(['0x%016x' % x for x in refinement_argument])
  print >>detail, 'refinement_expected:', ','.join(['0x%016x' % x for x in refinement_expected])
  detail.flush()

  logging.info('*** Running Cardinal System *** [ %d vs. %d ]',
               len(argument), len(refinement_argument))
  args = [FLAGS.cardinal_solver,
          '--size=%d' % problem.size,
          '--operators=%s' % ','.join(problem.operators),
          '--argument=%s' % ','.join(map(str, argument)),
          '--expected=%s' % ','.join(map(str, expected)),
          '--refinement_argument=%s' % ','.join(map(str, refinement_argument)),
          '--refinement_expected=%s' % ','.join(map(str, refinement_expected)),
          '--random_seed=%d' % random_seed,
          ]
  logging.info('command line: %s', ' '.join(args))
  proc = subprocess.Popen(args, stdout=subprocess.PIPE)
  if timeout_sec is not None:
    def TimeoutKiller():
      try:
        os.kill(proc.pid, signal.SIGXCPU)
      except:
        pass
    timer = threading.Timer(timeout_sec, TimeoutKiller)
    timer.start()
  output = proc.communicate(None)[0]
  if timeout_sec is not None:
    timer.cancel()
  if proc.returncode != 0:
    return None
  program = output.strip()
  assert program, 'no output from Cardinal'
  return program


def Guess(problem, program, detail):
  logging.info('=== %s', program)
  print >>detail, 'program:', program
  detail.flush()

  example = api.Guess(problem.id, program)
  if example:
    logging.info('rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x',
                 example.argument, example.expected, example.actual)
    print >>detail, '=> rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x' % (
        example.argument, example.expected, example.actual)
    detail.flush()
    return example
  logging.info('rejected, but could not get a counterexample.')
  print >>detail, '=> rejected, but could not get a counterexample.'
  detail.flush()
  return None


INITIAL_WAIT = 5
INITIAL_WAIT_INCREASE = 5
BUCKETIZE_WAIT_INIT = 5
BUCKETIZE_WAIT_INCREASE = 5


def SolveInternal(problem, random_io_pairs, detail, start_time, initial_wait=INITIAL_WAIT, bucketize_wait=BUCKETIZE_WAIT_INIT):
  refinement_argument = []
  refinement_expected = []

  trial = 0
  while True:
    if FLAGS.time_limit_sec is not None and time.time() - start_time > FLAGS.time_limit_sec:
      raise api.Expired('artificial time limit')

    # Sample 3 I/O randomly
    argument = []
    expected = []
    for i, o in random.sample(random_io_pairs, FLAGS.initial_arguments):
      argument.append(i)
      expected.append(o)

    program = RunCardinalSolver(
        problem, argument, expected,
        refinement_argument, refinement_expected, detail, initial_wait)
    if program:
      # YES!!! We got some program!!!!!!!
      example = Guess(problem, program, detail)
      if example:
        break
      # aaaaaaaaaa no example

    trial += 1
    if trial % 5 == 0:
      initial_wait += INITIAL_WAIT_INCREASE

  buckets = [(argument, expected), (refinement_argument, refinement_expected)]

  while True:
    if FLAGS.time_limit_sec is not None and time.time() - start_time > FLAGS.time_limit_sec:
      raise api.Expired('artificial time limit')

    # example docchi ni ireru???
    buckets.sort(key=lambda (a, e): -len(a))

    # ookii hou kara
    buckets[0][0].append(example.argument)
    buckets[0][1].append(example.expected)

    program = RunCardinalSolver(
        problem, argument, expected,
        refinement_argument, refinement_expected, detail, bucketize_wait)
    if program:
      example = Guess(problem, program, detail)
      if not example:
        return SolveInternal(problem, random_io_pairs, detail, start_time, initial_wait, bucketize_wait)
      continue

    # damedatta... orz
    buckets[0][0].pop()
    buckets[0][1].pop()

    # chiisai hou
    buckets[1][0].append(example.argument)
    buckets[1][1].append(example.expected)

    program = RunCardinalSolver(
        problem, argument, expected,
        refinement_argument, refinement_expected, detail, bucketize_wait)
    if program:
      example = Guess(problem, program, detail)
      if not example:
        return SolveInternal(problem, random_io_pairs, detail, start_time, initial_wait, bucketize_wait)
      continue

    # docchi mo dame datta... orzorz
    return SolveInternal(problem, random_io_pairs, detail, start_time,
                         initial_wait, bucketize_wait+BUCKETIZE_WAIT_INCREASE)


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
  random_io_pairs = known_io_pairs[-110:]

  print >>detail, '=== First Eval ==='
  for argument, output in zip(arguments, outputs):
    print >>detail, '0x%016x => 0x%016x' % (argument, output)
  detail.flush()

  try:
    SolveInternal(problem, random_io_pairs, detail, time.time())
  except api.Solved:
    logging.info('')
    logging.info(u'*\u30fb\u309c\uff9f\uff65*:.\uff61. '
                 u'SOLVED!'
                 u' .\uff61.:*\uff65\u309c\uff9f\uff65*:')
    logging.info('')
    print >>detail, '=> success.'
    detail.flush()
    return


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

    try:
      with open(os.path.join(FLAGS.detail_log_dir, '%s.txt' % problem.id), 'w') as detail:
        Solve(problem, detail)
    except api.Expired:
      logging.error('')
      logging.error(' !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ')
      logging.error('')
      logging.error('        P R O B L E M  E X P I R E D')
      logging.error('')
      logging.error(' !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ')
      logging.error('')
      if not FLAGS.keep_going:
        raise


if __name__ == '__main__':
  main()
