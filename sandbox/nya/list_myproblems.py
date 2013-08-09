from util import api


def main():
  ps = api.MyProblems()
  for p in ps:
    print '\t'.join(map(str, [p.id, p.size, ','.join(p.operators)]))


if __name__ == '__main__':
  main()
