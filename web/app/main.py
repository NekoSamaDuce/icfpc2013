import os

import bottle
import jinja2


jinja2_env = jinja2.Environment(
    loader=jinja2.FileSystemLoader(
        os.path.join(os.path.dirname(__file__), os.pardir, 'templates')),
    autoescape=True,
    extensions=['jinja2.ext.autoescape'])


def Render(template_name, template_dict=None):
  template_dict = template_dict or {}
  template = jinja2_env.get_template(template_name)
  return template.render(**template_dict)


@bottle.get('/')
def IndexHandler():
  return Render('index.html')


@bottle.get(r'/static/<path:path>')
def static_handler(path):
  return bottle.static_file(
      path, root=os.path.join(os.path.dirname(__file__), os.pardir, 'static'))


application = bottle.default_app()

if __name__ == '__main__':
  bottle.debug()
  bottle.run(host='0.0.0.0', port=8080, reloader=True, interval=1)
