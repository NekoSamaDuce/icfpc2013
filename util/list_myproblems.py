from util import api


def main():
  problems = api.MyProblems()
  problems.sort(key=lambda p: (p.size, p.id))
  for p in problems:
    line = p.ToProblemLine()
    if p.solved is None:
      print line
    elif p.solved:
      print '#SUCCESS:%s' % line
    else:
      print '#FAILED:%s' % line


if __name__ == '__main__':
  main()
