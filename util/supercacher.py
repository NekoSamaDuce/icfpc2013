import logging
import pymongo
import sys

import gflags

from util import stdlog

FLAGS = gflags.FLAGS

INABA_KEY = [
  0xfffffffffffffff9, 0xfffffffffffffffa, 0xfffffffffffffffb, 0xfffffffffffffffc,
  0xfffffffffffffffd, 0xfffffffffffffffe, 0xffffffffffffffff, 0x0000000000000000,
  0x0000000000000001, 0x0000000000000002, 0x0000000000000003, 0x0000000000000004,
  0x0000000000000005, 0x0000000000000006, 0x0000000000000007, 0x0000000000000001,
  0xfffffffffffffffe, 0x0000000000000002, 0xfffffffffffffffd, 0x0000000000000004,
  0xfffffffffffffffb, 0x0000000000000008, 0xfffffffffffffff7, 0x0000000000000010,
  0xffffffffffffffef, 0x0000000000000020, 0xffffffffffffffdf, 0x0000000000000040,
  0xffffffffffffffbf, 0x0000000000000080, 0xffffffffffffff7f, 0x0000000000000100,
  0xfffffffffffffeff, 0x0000000000000200, 0xfffffffffffffdff, 0x0000000000000400,
  0xfffffffffffffbff, 0x0000000000000800, 0xfffffffffffff7ff, 0x0000000000001000,
  0xffffffffffffefff, 0x0000000000002000, 0xffffffffffffdfff, 0x0000000000004000,
  0xffffffffffffbfff, 0x0000000000008000, 0xffffffffffff7fff, 0x0000000000010000,
  0xfffffffffffeffff, 0x0000000000020000, 0xfffffffffffdffff, 0x0000000000040000,
  0xfffffffffffbffff, 0x0000000000080000, 0xfffffffffff7ffff, 0x0000000000100000,
  0xffffffffffefffff, 0x0000000000200000, 0xffffffffffdfffff, 0x0000000000400000,
  0xffffffffffbfffff, 0x0000000000800000, 0xffffffffff7fffff, 0x0000000001000000,
  0xfffffffffeffffff, 0x0000000002000000, 0xfffffffffdffffff, 0x0000000004000000,
  0xfffffffffbffffff, 0x0000000008000000, 0xfffffffff7ffffff, 0x0000000010000000,
  0xffffffffefffffff, 0x0000000020000000, 0xffffffffdfffffff, 0x0000000040000000,
  0xffffffffbfffffff, 0x0000000080000000, 0xffffffff7fffffff, 0x0000000100000000,
  0xfffffffeffffffff, 0x0000000200000000, 0xfffffffdffffffff, 0x0000000400000000,
  0xfffffffbffffffff, 0x0000000800000000, 0xfffffff7ffffffff, 0x0000001000000000,
  0xffffffefffffffff, 0x0000002000000000, 0xffffffdfffffffff, 0x0000004000000000,
  0xffffffbfffffffff, 0x0000008000000000, 0xffffff7fffffffff, 0x0000010000000000,
  0xfffffeffffffffff, 0x0000020000000000, 0xfffffdffffffffff, 0x0000040000000000,
  0xfffffbffffffffff, 0x0000080000000000, 0xfffff7ffffffffff, 0x0000100000000000,
  0xffffefffffffffff, 0x0000200000000000, 0xffffdfffffffffff, 0x0000400000000000,
  0xffffbfffffffffff, 0x0000800000000000, 0xffff7fffffffffff, 0x0001000000000000,
  0xfffeffffffffffff, 0x0002000000000000, 0xfffdffffffffffff, 0x0004000000000000,
  0xfffbffffffffffff, 0x0008000000000000, 0xfff7ffffffffffff, 0x0010000000000000,
  0xffefffffffffffff, 0x0020000000000000, 0xffdfffffffffffff, 0x0040000000000000,
  0xffbfffffffffffff, 0x0080000000000000, 0xff7fffffffffffff, 0x0100000000000000,
  0xfeffffffffffffff, 0x0200000000000000, 0xfdffffffffffffff, 0x0400000000000000,
  0xfbffffffffffffff, 0x0800000000000000, 0xf7ffffffffffffff, 0x1000000000000000,
  0xefffffffffffffff, 0x2000000000000000, 0xdfffffffffffffff, 0x4000000000000000,
  0xbfffffffffffffff, 0x8000000000000000, 0x7fffffffffffffff, 0x864bf0b6263ff2dd,
  0x09d1f6033d7a320f, 0xbf638cdc79c2586c, 0xbb37e70985765193, 0xe066090edc5c4a1b,
  0xf533d3828ece66f7, 0x8e35e05cf3173771, 0xdf9c908e1634166f, 0xe172b7ae9f69e73a,
  0x0bdc863ccda2b00a, 0x1334522363856fb7, 0x0b581539970382cc, 0x59a4f49633066388,
  0x532eece07126c2ad, 0xd0dfeecf498f9d2f, 0x541e2e995767ae41, 0xc8859e3309ebc833,
  0x22c6d5ab67f96955, 0xd774665ab251b0d5, 0x65d6c2c61b1df131, 0xeeeb3202f6a81c9e,
  0xf200b4997eb46997, 0xb60fa37455c49824, 0x4d5c1c2cd20a9dca, 0x1ae95dd45a3177b6,
  0x41ffb8310d95ce55, 0x26595de5a1ac989e, 0xb44bd35fbdc27146, 0x0dec4818c29f0fec,
  0x2ce5005a93df414e, 0x130b2ccb09db03f5, 0x15271d36b119fad3, 0x959282e270ff82e9,
  0x83404b71b7a8f8bd, 0x8af4ae06b49af656, 0xd818d6b374f4cf6e, 0x4d6fb0904863e602,
  0xc2651eaa4c78ccb8, 0xcf652cb27b331904, 0xd2418f103d101053, 0xfa309cbca4b868d9,
  0x3335d7276a7802e7, 0xf0a6b228353e8c0d, 0x8e5121ca7993a6a6, 0x259577d6048d1ed1,
  0x490bdc92a734148e, 0xae0c98f43b324868, 0x00a76c386e7ef1d9, 0xf2670418e2efc788,
  0x7883e4e3b01609f6, 0xb9f1aba7681fdb2f, 0x302b4e7c491b9493, 0x121f9066c8682fe0,
  0x83103025d2bc0271, 0x929371f6f3a45f1d, 0x47385e375c10521b, 0xd043677fee9d92aa,
  0xac646f86baf67bb7, 0x664667cf2b70e772, 0x28f42cc3c08ac25a, 0xa48f2503b2334ec0,
  0x27d873249cd7cc24, 0x02fd1a6eb94a67a3, 0x888d67cdd529cb65, 0x9ecafd03b6b9ee22,
  0x71c66b689e8c4092, 0x3cbadb5e12a2a8e0, 0x233fb0c4d8111239, 0x4c98e67299093fba,
  0x59a1f7ab50db44e2, 0xfa998fd46f65e9f0, 0xf1e163dd83d35bdf, 0x946e7e421e7dd1dd,
  0x0653c611858605d8, 0x9b27ce2ebddbbb17, 0xa4535feebbdedf3b, 0x3fd9afa0042a87c0,
  0x1cb940711c6b9600, 0x9230a1ae4341dd9f, 0x4f807df8bbe73433, 0x1818cc277dfa2b8a,
  0x0e847c9e951b0b65, 0x3c6d43f22348dde2, 0x359563849dcfad66, 0xe119eb906a8b9837,
  0x08233d5a03205880, 0xe6bde24b47dbae85, 0x2491b0beb22e591d, 0x235082e73b81a2ea,
  0xf674e22515b4f78f, 0x7365cb0a082cd3a4, 0x5e0bab20690af7bc, 0xc75b11132b246edc,
  0x7f7a1c7375a2b6f7, 0x8bb84fcae2da2ec0, 0xbd37440b2a088f00, 0xa265c4596911ecf8,
  0x30cece991df4e0f9, 0x4aafb87850d7c25a, 0x9635af385fd63769, 0xffb8b34cf15fcc80,
  0xfdbddb25022e98cc, 0x9cdd72c0fedf3561, 0xffcbce4cb1114015, 0xbcd66a5a1bd5b3de,
  0x9340b491706f12d3, 0xde09e735fd565b5e, 0x0582b685d392ef10, 0xfc0a16c3beb4f770,
  0x429d832e75a08c67, 0xd1a8b09c3a028bc7, 0xc11063b838360990, 0x206e9c94d0548533,
  ]


def main():
  sys.argv = FLAGS(sys.argv)
  stdlog.setup()

  db = pymongo.Connection()
  db.unique_cache.cache.ensure_index(
      [('expected', pymongo.ASCENDING)])
  db.master_cache.cache.ensure_index(
      [('expected', pymongo.ASCENDING), ('program', pymongo.ASCENDING)])

  first_line = sys.stdin.readline()
  assert first_line.startswith('argument:')
  arguments = [int(s, 0) for s in first_line.split()[1].split(',')]
  assert arguments == INABA_KEY, 'not inaba key'

  print 'ok'

  for line in sys.stdin:
    line = line.strip()
    if line.startswith('expected:'):
      expected = [int(s, 0) for s in line.split()[1].split(',')]
    else:
      db.unique_cache.cache.update(
          {'expected': expected},
          {'$set': {'program': line}}, upsert=True)
      db.master_cache.cache.update(
          {'expected': expected, 'program': line},
          {'$set': {}}, upsert=True)


if __name__ == '__main__':
  main()
