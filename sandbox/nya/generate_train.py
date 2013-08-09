import sys
import time

import gflags

import api

FLAGS = gflags.FLAGS

gflags.DEFINE_integer('count', None, 'How many problems to generate?')
gflags.MarkFlagAsRequired('count')

gflags.DEFINE_integer('size', None, 'Problem size')
gflags.MarkFlagAsRequired('size')

gflags.DEFINE_string('operators', None, 'Problem operators')
gflags.MarkFlagAsRequired('operators')

gflags.DEFINE_integer('sleep', 5, 'Sleep between requests')


def main():
  sys.argv = FLAGS(sys.argv)
  for _ in xrange(FLAGS.count):
    p = api.Train(FLAGS.size, FLAGS.operators)
    print '\t'.join(map(str, [p.id, p.size, ','.join(p.operators), p.answer]))
    time.sleep(FLAGS.sleep)


if __name__ == '__main__':
  main()
