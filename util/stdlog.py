import codecs
import logging
import sys

_ENCODING = 'UTF-8'

_FORMATTER = logging.Formatter(
    fmt='%(asctime)-15s %(levelname)s [%(filename)s:%(lineno)d] %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S')


class Utf8StreamHandler(logging.StreamHandler):
  def __init__(self, stream):
    stream = codecs.getwriter(_ENCODING)(stream)
    logging.StreamHandler.__init__(self, stream)


def setup():
  root = logging.getLogger()
  root.setLevel(logging.INFO)
  stderr_handler = Utf8StreamHandler(sys.stderr)
  stderr_handler.setFormatter(_FORMATTER)
  root.addHandler(stderr_handler)
