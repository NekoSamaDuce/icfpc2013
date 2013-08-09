import subprocess
import sys
import time

import gflags

from util import api

FLAGS = gflags.FLAGS

gflags.DEFINE_string('id', None, 'Problem ID to solve')
gflags.MarkFlagAsRequired('id')

gflags.DEFINE_string('solver', 'sandbox/kinaba/zentansaku', 'Path to solver')
gflags.MarkFlagAsRequired('solver')

gflags.DEFINE_string('problems_file', None, 'Path to problems file')
gflags.MarkFlagAsRequired('problems_file')


def main():
  sys.argv = FLAGS(sys.argv)

  problems = {}
  with open(FLAGS.problems_file) as f:
    for line in f:
      id, size, operators = line.rstrip('\n').split('\t')[:3]
      size = int(size)
      operators = operators.split(',')
      problem = api.Problem(id, size, operators)
      problems[problem.id] = problem

  assert FLAGS.id in problems, 'Unknown --id specified'
  problem = problems[FLAGS.id]

  solver_output = subprocess.check_output(
      [FLAGS.solver, '--size=%d' % problem.size,
       '--operators=%s' % ','.join(problem.operators)])
  programs = solver_output.splitlines()

  for program in programs:
    print '===', program
    for trial in xrange(60):
      try:
        example = api.Guess(FLAGS.id, program)
        print 'rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x' % (
            example.argument, example.expected, example.actual)
        break
      except api.RateLimited:
        print '... rate limited ...'
        if trial > 0:
          time.sleep(1)
      except api.Solved:
        print '!!! SOLVED !!!'
        sys.exit(0)
  print 'NO PROGRAM WAS ACCEPTED. DEAD END...'


if __name__ == '__main__':
  main()
