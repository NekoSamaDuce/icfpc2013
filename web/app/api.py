import json

import requests

AUTH_TOKEN = '__REDACTED__'


class ApiError(Exception):
  pass


class RateLimited(ApiError):
  pass


class Solved(Exception):
  pass


class Problem(object):
  def __init__(self, id, size, operators, answer=None, solved=None, time_left=None):
    self.id = id
    self.size = size
    self.operators = operators
    self.answer = answer
    self.solved = solved
    self.time_left = time_left

  def AsDict(self):
    data = {
      'id': self.id,
      'size': self.size,
      'operators': self.operators,
      }
    if self.answer is not None:
      data['answer'] = self.answer
    if self.solved is not None:
      data['solved'] = self.solved
    if self.time_left is not None:
      data['timeLeft'] = self.time_left
    return data

  def __repr__(self):
    v = ['Problem']
    v.append('id=%r' % self.id)
    v.append('size=%r' % self.size)
    v.append('operators=%r' % self.operators)
    if self.answer is not None:
      v.append('answer=%r' % self.answer)
    if self.solved is not None:
      v.append('solved=%r' % self.solved)
    if self.time_left is not None:
      v.append('time_left=%r' % self.time_left)
    return '<%s>' % ' '.join(v)


class CounterExample(object):
  def __init__(self, argument, expected, actual):
    self.argument = argument
    self.expected = expected
    self.actual = actual

  def __repr__(self):
    v = ['CounterExample']
    v.append('argument=%r' % self.argument)
    v.append('expected=%r' % self.expected)
    v.append('actual=%r' % self.actual)
    return '<%s>' % ' '.join(v)


def GenericRequest(endpoint, request=None):
  url = 'http://icfpc2013.cloudapp.net/%s?auth=%s' % (endpoint, AUTH_TOKEN)
  if request is None:
    response = requests.post(url)
  else:
    response = requests.post(url, data=json.dumps(request))
  if response.status_code == 200:
    return response.json()
  if response.status_code == 429:
    raise RateLimited()
  raise ApiError('status_code = %d' % response.status_code)


def Train(size, operators):
  assert 3 <= size <= 30, 'size should be in [3, 30]'
  assert operators in ('', 'tfold', 'fold'), (
    'operators must be either an empty, tfold or fold')
  request = {'size': size, 'operators': operators}
  p = GenericRequest('train', request)
  return Problem(str(p['id']), p['size'], map(str, p['operators']),
                 answer=str(p['challenge']))


def MyProblems():
  #raise Exception('Do not access real data!')
  ps = GenericRequest('myproblems')
  return [Problem(str(p['id']), p['size'], map(str, p['operators']),
                  solved=p.get('solved'), time_left=p.get('timeLeft'))
          for p in ps]


def Eval(id, arguments):
  assert 1 <= len(arguments) <= 256, '# of arguments should be in [1, 256]'
  request = {'id': id, 'arguments': ['0x%016X' % a for a in arguments]}
  s = GenericRequest('eval', request)
  if s['status'] == 'error':
    raise ApiError(str(s['message']))
  assert s['status'] == 'ok'
  return [int(o, 0) for o in s['outputs']]


def Guess(id, program):
  request = {'id': id, 'program': program}
  s = GenericRequest('guess', request)
  if s['status'] == 'error':
    raise ApiError(str(s['message']))
  if s['status'] == 'win':
    raise Solved(id)
  assert s['status'] == 'mismatch'
  return CounterExample(*[int(x, 0) for x in s['values']])
