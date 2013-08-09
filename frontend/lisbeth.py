"""Lisbeth: Solver frontend v1.

Example:
./python.sh -m frontend.lisbeth --mode=train --size=3 --operators=
./python.sh -m frontend.lisbeth --mode=serious --problemset_file=data/train_small.tsv --problem_id=xop6dUzEtUBAprA0DGVwcAvB
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
    'genall_solver', os.path.join(SOLVER_DIR, 'genall'),
    'Path to genall solver binary.')
gflags.MarkFlagAsRequired('genall_solver')


def main():
  sys.argv = FLAGS(sys.argv)
  stdlog.setup()

  # Solver existence checks
  assert os.path.exists(FLAGS.genall_solver)

  problem = frontend_util.GetProblemByFlags()

  solver_output = subprocess.check_output(
      [FLAGS.genall_solver,
       '--size=%d' % problem.size,
       '--operators=%s' % ','.join(problem.operators)])
  programs = solver_output.splitlines()

  logging.info('Candidate programs: %d', len(programs))

  for program in programs:
    logging.info('=== %s', program)
    for trial in xrange(60):
      try:
        example = api.Guess(problem.id, program)
        logging.info('rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x',
                     example.argument, example.expected, example.actual)
        break
      except api.RateLimited:
        logging.info('... rate limited ...')
        if trial > 0:
          time.sleep(1)
      except api.Solved:
        logging.info('')
        logging.info(u'*\u30fb\u309c\uff9f\uff65*:.\uff61. '
                     u'SOLVED!'
                     u' .\uff61.:*\uff65\u309c\uff9f\uff65*:')
        logging.info('')
        sys.exit(0)
  logging.info('************************************')
  logging.info('NO PROGRAM WAS ACCEPTED. DEAD END...')
  logging.info('************************************')
  sys.exit(1)


if __name__ == '__main__':
  main()