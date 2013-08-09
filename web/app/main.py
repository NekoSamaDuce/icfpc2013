import json
import os
import time

import bottle
import jinja2

import api

jinja2_env = jinja2.Environment(
    loader=jinja2.FileSystemLoader(
        os.path.join(os.path.dirname(__file__), os.pardir, 'templates')),
    autoescape=True,
    extensions=['jinja2.ext.autoescape'])


def TextAbort(code, msg):
  raise bottle.HTTPResponse(msg, status=code)


def Render(template_name, template_dict=None):
  template_dict = template_dict.copy() if template_dict else {}
  template_dict['current_time'] = time.time()
  template = jinja2_env.get_template(template_name)
  return template.render(**template_dict)


@bottle.get('/')
def IndexHandler():
  return Render('index.html')


@bottle.get('/training')
def TrainingHandler():
  return Render('training.html')


@bottle.post('/training/generate')
def TrainingGenerateHandler():
  size = int(bottle.request.POST['size'])
  operators = str(bottle.request.POST['operators'])
  try:
    problem = api.Train(size, operators)
  except AssertionError as e:
    TextAbort(400, 'bad request: %s' % e)
  except api.ApiError as e:
    TextAbort(400, str(e))
  return problem.AsDict()


@bottle.post('/training/eval')
def TrainingEvalHandler():
  id = bottle.request.POST['id']
  argument_raw = str(bottle.request.POST['argument'])
  arguments = []
  for token in argument_raw.split(','):
    token = token.strip()
    if not token:
      continue
    try:
      arguments.append(int(token, 0))
    except ValueError:
      TextAbort(400, 'bad argument: %s' % token)
  try:
    outputs = api.Eval(id, arguments)
  except AssertionError as e:
    TextAbort(400, 'bad request: %s' % e)
  except api.ApiError as e:
    TextAbort(400, str(e))
  return {'cases': [{'argument': '0x%016X' % argument, 'output': '0x%016X' % output}
                    for argument, output in zip(arguments, outputs)]}


@bottle.post('/training/guess')
def TrainingGuessHandler():
  id = bottle.request.POST['id']
  program = str(bottle.request.POST['program'])
  try:
    example = api.Guess(id, program)
  except AssertionError as e:
    TextAbort(400, 'bad request: %s' % e)
  except api.Solved:
    TextAbort(403, 'win!')
  except api.ApiError as e:
    TextAbort(400, str(e))
  return {'status': 'mismatch',
          'argument': '0x%016X' % example.argument,
          'expected': '0x%016X' % example.expected,
          'actual': '0x%016X' % example.actual,
          }


@bottle.get(r'/static/<path:path>')
def static_handler(path):
  return bottle.static_file(
      path, root=os.path.join(os.path.dirname(__file__), os.pardir, 'static'))


application = bottle.default_app()

if __name__ == '__main__':
  bottle.debug()
  bottle.run(host='0.0.0.0', port=8080, reloader=True, interval=1)
