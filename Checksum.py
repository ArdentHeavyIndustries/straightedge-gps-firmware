#!/usr/bin/python

import sys

def checksum(bs):
    ckA = 0
    ckB = 0
    for b in bs:
        ckA = (ckA + ord(b)) & 0xff
        ckB = (ckB + ckA) & 0xff
    return (ckA, ckB)

def hexByte(b):
    if (b >= 16):
        return hex(b)[2:4]
    else:
        return ("0" + hex(b)[2:3])

syncByteLen = 2

if len(sys.argv) != 2:
    sys.stderr.write("USAGE: %s <INPUT>\n" % sys.argv[0])
    sys.exit(2)

f = open(sys.argv[1], "rb")
packet = f.read()
f.close()

sys.stderr.write("Read packet of %d bytes, not checksumming the first %d\n" % (len(packet), syncByteLen))

(checkA, checkB) = checksum(packet[syncByteLen:])

n = 0
for b in packet:
    sys.stdout.write(hexByte(ord(b)))
    if ((n % 8) == 7):
        sys.stdout.write("\n")
    else:
        sys.stdout.write(" ")

sys.stdout.write("\nChecksum: %s %s\n" % (hexByte(checkA), hexByte(checkB)))

outname = sys.argv[1] + "-chk"

g = open(outname, "wb")
g.write(packet)
g.write(chr(checkA))
g.write(chr(checkB))
g.close()

sys.stderr.write("Wrote checksummed packet to " + outname + "\n")
