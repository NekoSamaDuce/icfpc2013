import sys
import time

import gflags

from util import api

FLAGS = gflags.FLAGS

gflags.DEFINE_integer('count', 1, 'How many problems to generate?')
gflags.MarkFlagAsRequired('count')

gflags.DEFINE_integer('size', None, 'Problem size')
gflags.MarkFlagAsRequired('size')

gflags.DEFINE_string('operators', None, 'Problem operators')
gflags.MarkFlagAsRequired('operators')

gflags.DEFINE_integer('sleep', 5, 'Sleep between requests')
gflags.MarkFlagAsRequired('sleep')


def main():
  sys.argv = FLAGS(sys.argv)
  for i in reversed(xrange(FLAGS.count)):
    p = api.Train(FLAGS.size, FLAGS.operators)
    print '%s\t%s' % (p.ToProblemLine(), p.answer)
    if i > 0:
      time.sleep(FLAGS.sleep)


if __name__ == '__main__':
  main()
