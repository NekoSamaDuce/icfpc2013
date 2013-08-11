"""Kirito the Black Swordsman.

Example:
./python.sh -m frontend.kirito --cardinal_solver=solver/cardinal --detail_log_dir=details --mode=train --size=10 --operators= --train_count=3
"""

import logging
import os
import random
import signal
import StringIO
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
    'alice_solver', None,
    'Path to alice solver binary.')
gflags.MarkFlagAsRequired('alice_solver')

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


class Alice(object):
  def __init__(self):
    self.proc = subprocess.Popen(
        [FLAGS.alice_solver],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE)
    self._WaitReady()

  def _WaitReady(self):
    logging.info('Alice is booting. This can take a while and consumes a lot of RAM.')
    msg = self.proc.stdout.readline().strip()
    assert msg == 'ready', msg
    logging.info('Alice READY!')

  def Request(self, problem, else_argument, else_expected, then_argument, then_expected,
              first_request, timeout_sec, detail):
    random_seed = random.randrange(0, 1000000)

    print >>detail, ''
    print >>detail, '=== Alice Request ==='
    print >>detail, 'random_seed:', random_seed
    print >>detail, 'else_argument:', ','.join(['0x%016x' % x for x in else_argument])
    print >>detail, 'else_expected:', ','.join(['0x%016x' % x for x in else_expected])
    print >>detail, 'then_argument:', ','.join(['0x%016x' % x for x in then_argument])
    print >>detail, 'then_expected:', ','.join(['0x%016x' % x for x in then_expected])
    detail.flush()

    logging.info('*** Running Alice *** [ %d vs. %d ] T/O=%d',
                 len(else_argument), len(then_argument), timeout_sec)

    request = StringIO.StringIO()
    print >>request, 'request1'
    print >>request, 0 if first_request else 1
    print >>request, timeout_sec
    print >>request, problem.size
    print >>request, ','.join(problem.operators)
    print >>request, ','.join(map(str, else_argument))
    print >>request, ','.join(map(str, else_expected))
    print >>request, ','.join(map(str, then_argument))
    print >>request, ','.join(map(str, then_expected))
    print >>request, random_seed
    request = request.getvalue()

    print >>detail, 'protocol dump:'
    detail.write(request)
    detail.flush()

    self.proc.stdin.write(request)
    self.proc.stdin.flush()

    program = self.proc.stdout.readline().strip()
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


def SolveInternal(problem, random_io_pairs, alice, detail,
                  start_time,
                  initial_wait=INITIAL_WAIT, bucketize_wait=BUCKETIZE_WAIT_INIT):
  then_argument = []
  then_expected = []
  first_request = True

  trial = 0
  while True:
    if FLAGS.time_limit_sec is not None and time.time() - start_time > FLAGS.time_limit_sec:
      raise api.Expired('artificial time limit')
    # Sample 3 I/O randomly
    else_argument = []
    else_expected = []
    for i, o in random.sample(random_io_pairs, FLAGS.initial_arguments):
      else_argument.append(i)
      else_expected.append(o)

    program = alice.Request(
        problem, else_argument, else_expected, then_argument, then_expected,
        first_request, initial_wait, detail)
    first_request = False
    if program:
      # YES!!! We got some program!!!!!!!
      example = Guess(problem, program, detail)
      if example:
        break
      # aaaaaaaaaa no example

    trial += 1
    if trial >= 5:
      initial_wait += INITIAL_WAIT_INCREASE
      bucketize_wait = max(bucketize_wait, initial_wait)

  buckets = [(else_argument, else_expected), (then_argument, then_expected)]

  while True:
    if FLAGS.time_limit_sec is not None and time.time() - start_time > FLAGS.time_limit_sec:
      raise api.Expired('artificial time limit')

    # example docchi ni ireru???
    buckets.sort(key=lambda (a, e): -len(a))

    # ookii hou kara
    buckets[0][0].append(example.argument)
    buckets[0][1].append(example.expected)

    program = alice.Request(
        problem, else_argument, else_expected, then_argument, then_expected,
        first_request, bucketize_wait, detail)
    first_request = False
    if program:
      example = Guess(problem, program, detail)
      if not example:
        return SolveInternal(problem, random_io_pairs, alice, detail,
                             start_time,
                             initial_wait, bucketize_wait)
      continue

    # damedatta... orz
    buckets[0][0].pop()
    buckets[0][1].pop()

    # chiisai hou
    buckets[1][0].append(example.argument)
    buckets[1][1].append(example.expected)

    program = alice.Request(
        problem, else_argument, else_expected, then_argument, then_expected,
        first_request, bucketize_wait, detail)
    first_request = False
    if program:
      example = Guess(problem, program, detail)
      if not example:
        return SolveInternal(problem, random_io_pairs, alice, detail,
                             start_time,
                             initial_wait, bucketize_wait)
      continue

    # docchi mo dame datta... orzorz
    return SolveInternal(problem, random_io_pairs, alice, detail,
                         start_time,
                         initial_wait, bucketize_wait+BUCKETIZE_WAIT_INCREASE)


def Solve(problem, alice, detail):
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
    start_time = time.time()
    SolveInternal(problem, random_io_pairs, alice, detail, start_time)
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
  assert os.path.exists(FLAGS.alice_solver)

  problems = frontend_util.GetProblemsByFlags()

  alice = Alice()

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
        Solve(problem, alice, detail)
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
