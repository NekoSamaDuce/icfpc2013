import sys
import time

import gflags

from util import api

FLAGS = gflags.FLAGS

gflags.DEFINE_string('id', None, 'Problem ID to solve')
gflags.MarkFlagAsRequired('id')

gflags.DEFINE_string('guess_file', None, 'Path to guess file')
gflags.MarkFlagAsRequired('guess_file')


def main():
  sys.argv = FLAGS(sys.argv)

  with open(FLAGS.guess_file) as f:
    for program in f:
      program = program.strip()
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
