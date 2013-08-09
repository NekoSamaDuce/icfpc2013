import logging

import gflags

from util import api

FLAGS = gflags.FLAGS

gflags.DEFINE_enum(
    'mode', None, ['train', 'oneoff', 'serious'],
    'Specifies how the frontend reads a problem. '
    '"train" generates a training problem automatically beforehand with '
    '--size and --operators. "serious" picks up a problem from a TSV file '
    'specified by --problemset_file with ID specified by --id.')
gflags.MarkFlagAsRequired('mode')

# Flags for --mode=train / --mode=oneoff
gflags.DEFINE_integer(
    'size', None,
    'The training problem size to generate.')
gflags.DEFINE_string(
    'operators', None,
    'The training problem operators to generate.')

# Flags for --mode=serious
gflags.DEFINE_string(
    'problemset_file', None,
    'Path to the problemset TSV file.')

# Flags for --mode=oneoff / --mode=oneoff
gflags.DEFINE_string(
    'problem_id', None,
    'The problem ID to pick from the problem set specified by '
    '--problemset_file.')


def GetProblemByFlags():
  if FLAGS.mode == 'train':
    logging.info('Requesting a training problem...')
    problem = api.Train(FLAGS.size, FLAGS.operators)
  elif FLAGS.mode == 'oneoff':
    assert FLAGS.problem_id is not None
    assert FLAGS.size is not None
    assert FLAGS.operators is not None
    problem = api.Problem(FLAGS.problem_id, FLAGS.size, FLAGS.operators.split(','))
  else:  # FLAGS.mode == 'serious'
    assert FLAGS.problem_id, '--problem_id must be specified'
    with open(FLAGS.problemset_file) as f:
      for line in f:
        id, size, operators = line.rstrip('\n').split('\t')[:3]
        size = int(size)
        operators = operators.split(',')
        problem = api.Problem(id, size, operators)
        if problem.id == FLAGS.problem_id:
          break
      else:
        raise AssertionError('No problem with ID=%s found' % FLAGS.problem_id)
  logging.info('Problem loaded.')
  logging.info('ID: %s', problem.id)
  logging.info('Size: %d', problem.size)
  logging.info('Operators: %s', ', '.join(problem.operators))
  return problem
