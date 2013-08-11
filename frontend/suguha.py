"""Suguha: Solver frontend v2+ for bonus-problem type normal problems.

Example:
./python.sh -m frontend.suguha --cluster_solver=solver/simplify_main --batch_evaluate_solver=solver/batch_evaluate --mode=train --size=3 --operators= --max_cluster_size=20 --counterexample_filter=false --max_component_size=8
./python.sh -m frontend.suguha --cluster_solver=solver/simplify_main --batch_evaluate_solver=solver/batch_evaluate --mode=serious --problemset_file=data/train_small.tsv --problem_id=xop6dUzEtUBAprA0DGVwcAvB --counterexample_filter=false --max_component_size=8
"""

import logging
import os
import random
import subprocess
import sys
import time

import gflags

from frontend import frontend_util
from util import api
from util import stdlog

FLAGS = gflags.FLAGS

BAILOUT_FILE = os.path.join(os.path.dirname(__file__), os.pardir, 'BAILOUT')

gflags.DEFINE_string(
    'cluster_solver', None,
    'Path to cluster solver binary.')
gflags.MarkFlagAsRequired('cluster_solver')

gflags.DEFINE_string(
    'batch_evaluate_solver', None,
    'Path to batch evaluate solver binary.')
gflags.MarkFlagAsRequired('batch_evaluate_solver')

gflags.DEFINE_boolean(
    'counterexample_filter', True,
    'Call batch_evaluate to filter candidates with counterexamples.')

gflags.DEFINE_integer(
    'max_component_size', None,
    'Maximum size of component expressions.')
gflags.MarkFlagAsRequired('max_component_size')

# i-th bit is '1' if 'then' branch is taken for the input.
# '0' if 'else' branch is taken.
def AsConditionBitVector(output):
  bv = 0
  for i,v in enumerate(output):
    bv |= (1 if v==0 else 0) << i
  return bv

def ComputeAtteruBitVector(cluster_output, server_output):
  bv = 0
  for i,(co,so) in enumerate(zip(cluster_output, server_output)):
    bv |= (1 if co==so else 0) << i
  return bv

def ConstructProgram(cte):
  c,t,e = cte
  def cut(s):
    return s[12:-1]
  return "(lambda (x) (if0 %s %s %s))" % (cut(c),cut(t),cut(e))

def RunClusterSolver(problem):
  operators_but_bonus = problem.operators[:]
  if 'bonus' in operators_but_bonus:
    operators_but_bonus.remove('bonus')
  logging.info('Running clusterer...')
  cluster_solver_output = subprocess.check_output(
      [FLAGS.cluster_solver,
       '--size=%d' % (FLAGS.max_component_size  + 1),
       '--operators=%s' % ','.join(operators_but_bonus)])
  logging.info('Finished.')
  assert cluster_solver_output.startswith('argument: ')
  lines = cluster_solver_output.splitlines()
  arguments = [int(s, 0) for s in lines.pop(0).split()[1].split(',')]
  clusters = []
  for line in lines:
    if line.startswith('expected:'):
      expected = [int(s, 0) for s in line.split()[1].split(',')]
      clusters.append([expected, [], AsConditionBitVector(expected), None])
    else:
      clusters[-1][1].append(line)
  return (arguments, clusters)


def BruteForceGuessOrDie(problem, programs):
  while programs:
    program = programs.pop(0)
    logging.info('=== [%d] %s', len(programs) + 1, program)
    try:
      example = api.Guess(problem.id, program)
    except api.Solved:
      logging.info('')
      logging.info(u'*\u30fb\u309c\uff9f\uff65*:.\uff61. '
                   u'SOLVED!'
                   u' .\uff61.:*\uff65\u309c\uff9f\uff65*:')
      logging.info('')
      return
    if example:
      logging.info('rejected. argument=0x%016x, expected=0x%016x, actual=0x%016x',
                   example.argument, example.expected, example.actual)
      if FLAGS.counterexample_filter:
        programs = FilterProgramsWithCounterExample(programs, example)
    else:
      logging.info('rejected, but could not get a counterexample.')
  logging.error('************************************')
  logging.error('NO PROGRAM WAS ACCEPTED. DEAD END...')
  logging.error('************************************')
  sys.exit(1)


def FilterProgramsWithCounterExample(programs, example):
  if not programs:
    return []
  logging.info('calling batch_evaluate to filter candidates.')
  p = subprocess.Popen(
      [FLAGS.batch_evaluate_solver,
       '--argument=%d' % example.argument],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE)
  evaluate_output = p.communicate('\n'.join(programs))[0]
  if p.returncode != 0:
    logging.error('batch_evaluate failed. Continuing without filter anyway...')
    return programs
  new_programs = []
  for program, output in zip(programs, evaluate_output.splitlines()):
    output = int(output, 0)
    if output == example.expected:
      new_programs.append(program)
  return new_programs


def main():
  sys.argv = FLAGS(sys.argv)
  stdlog.setup()

  # Solver existence checks
  assert os.path.exists(FLAGS.cluster_solver)
  assert os.path.exists(FLAGS.batch_evaluate_solver)

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

    arguments, clusters = RunClusterSolver(problem)

    cluster_sizes_decreasing = sorted(
        [len(programs) for _, programs, _, _ in clusters], reverse=True)
    logging.info('Candidate programs: %d', sum(cluster_sizes_decreasing))
    logging.info('Candidate clusters: %d', len(clusters))
    #logging.info('Cluster sizes: %s', ', '.join(map(str, cluster_sizes_decreasing)))

    logging.info('Issueing /eval...')
    outputs = api.Eval(problem.id, arguments)

    for c in clusters:
      c[3] = ComputeAtteruBitVector(c[0], outputs)

    # Make mapping by bit vectors.
    c_map = dict()
    for (_,p,c_bv,t_bv) in clusters:
      c_map.setdefault(c_bv, []).append(p[0])
    t_map = dict()
    for (_,p,c_bv,t_bv) in clusters:
      t_map.setdefault(t_bv, []).append(p[0])
    logging.info('C-T-E finder loop for %d (as-cond-pat: %d, as-te-pat: %d) clusters...',
        len(clusters), len(c_map), len(t_map))

    fullbits = (1 << len(outputs)) - 1
    cand = []
    try_counter_example_limit = 10000

    high_t_map = dict()
    for (t_bv, t_progs) in t_map.items():
      bitnum = bin(t_bv).count('1')
      if bitnum >= (len(outputs)+1)/2:
        high_t_map[t_bv] = t_progs
      if bitnum == len(outputs):
        for t in t_progs:
          cand.append(("(lambda (x) 0)",t,t))

    found_fullhit = (len(cand) > 0)
    if found_fullhit:
      logging.info("Full hit found: %d", len(cand))

    def cand_add(cs,ts,es):
      lim = 10
      if found_fullhit:
        lim = 3
      if len(cand) < try_counter_example_limit:
        lim = 3
      if len(cand) < try_counter_example_limit*10:
        lim = 1
      tss = ts if len(ts)<lim else random.sample(ts, lim)
      css = cs if len(cs)<lim else random.sample(cs, lim)
      ess = es if len(es)<lim else random.sample(es, lim)
      for c in css:
        for t in tss:
          for e in ess:
            cand.append((c, t, e))

    if len(high_t_map) < 500:
      # First filter by high_bit_candidates
      for (t_bv, t_progs) in high_t_map.items(): # find high bitters
        for (e_bv, e_progs) in t_map.items():
          if (t_bv | e_bv) == fullbits: # must cover all cases
            for (c_bv, c_progs) in c_map.items():
              c_bv_inv = ~c_bv & fullbits
              if (c_bv & t_bv) == c_bv and (c_bv_inv & e_bv) == c_bv_inv:  # c,t,e
                cand_add(c_progs, t_progs, e_progs)
              if (c_bv & e_bv) == c_bv and (c_bv_inv & t_bv) == c_bv_inv:  # c,e,t
                cand_add(c_progs, e_progs, t_progs)
    else:
      for (c_bv, c_progs) in c_map.items():
        ok_as_then = []
        ok_as_else = []
        c_bv_inv = ~c_bv & fullbits
        for (t_bv, t_progs) in t_map.items():
          if (c_bv & t_bv) == c_bv:
            ok_as_then.extend(t_progs)
          if (c_bv_inv & t_bv) == c_bv_inv:
            ok_as_else.extend(t_progs)
        cand_add(c_progs, ok_as_then, ok_as_else)

    logging.info("Candidate size: %d", len(cand))
    if len(cand) > try_counter_example_limit:
      cand = cand[0:30] + random.sample(cand[30:], try_counter_example_limit - 30)
      logging.info("Reduced candidate size: %d", len(cand))

    programs = [ConstructProgram(p) for p in cand]
    BruteForceGuessOrDie(problem, programs)

if __name__ == '__main__':
  main()
