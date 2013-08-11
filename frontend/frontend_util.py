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
gflags.DEFINE_integer(
    'train_count', None,
    'The number of training problems.')

# Flags for --mode=serious
gflags.DEFINE_string(
    'problemset_file', None,
    'Path to the problemset TSV file.')

# Flags for --mode=serious / --mode=oneoff
gflags.DEFINE_string(
    'problem_id', None,
    'If --mode=oneoff, specifies the problem ID. '
    'If --mode=serious, this specifies multiple problem IDs to '
    'process from the problemset file specified by --problemset_file. '
    'Problem ID specification can be comma-separated list of IDs or "ALL".')

# Flags for --mode=train
gflags.DEFINE_bool(
    'train_exclude_fold', False,
    'Exclude problems with fold')


def GetProblemsByFlags():
  if FLAGS.mode == 'train':
    assert FLAGS.train_count is not None, '--train_count must be specified'
    logging.info('Requesting a training problem...')
    problems = []
    while len(problems) < FLAGS.train_count:
      p = api.Train(FLAGS.size, FLAGS.operators)
      if FLAGS.train_exclude_fold and 'fold' in p.operators or 'tfold' in p.operators:
        continue
      problems.append(p)
  elif FLAGS.mode == 'oneoff':
    assert FLAGS.problem_id is not None
    assert FLAGS.size is not None
    assert FLAGS.operators is not None
    problems = [api.Problem(FLAGS.problem_id, FLAGS.size,
                            FLAGS.operators.split(','))]
  else:  # FLAGS.mode == 'serious'
    assert FLAGS.problem_id, '--problem_id must be specified'
    problemset = {}
    problem_ids = []
    with open(FLAGS.problemset_file) as f:
      for line in f:
        if line.startswith('#'):
          continue
        id, size, operators = line.rstrip('\n').split('\t')[:3]
        size = int(size)
        operators = operators.split(',')
        problem = api.Problem(id, size, operators)
        problemset[problem.id] = problem
        problem_ids.append(problem.id)
    if FLAGS.problem_id != 'ALL':
      problem_ids = FLAGS.problem_id.split(',')
    problems = [problemset[problem_id] for problem_id in problem_ids]
  logging.info('%d Problems loaded.', len(problems))
  for i, problem in enumerate(problems):
    logging.info('%d. %s size=%d operators=%s',
                 i+1, problem.id, problem.size, ','.join(problem.operators))
  if FLAGS.mode != 'train':
    logging.info('Enter "go" to continue.')
    while raw_input() != 'go':
      pass
  return problems
