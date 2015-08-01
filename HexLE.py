#!/usr/bin/python

import sys

if len(sys.argv) != 2:
    sys.stderr.write("USAGE: %s <DECIMAL NUMBER>\n" % sys.argv[0])
    sys.exit(2)

x = int(sys.argv[1])

sys.stdout.write("%d = %02x %02x %02x %02x\n" % (x, (x & 0xff), ((x >> 8) & 0xff), ((x >> 16) & 0xff), ((x >> 24) & 0xff)))
