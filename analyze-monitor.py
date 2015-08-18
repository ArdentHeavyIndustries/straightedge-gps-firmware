#!/usr/bin/python

import sys

baseTime = 0

if len(sys.argv) < 2 or len(sys.argv) > 3:
    sys.stderr.write("USAGE: %s <INPUT> [<BASETIME>]\n" % sys.argv[0])
    sys.exit(2)

if len(sys.argv) == 3:
    baseTime = int(sys.argv[2])

f = open(sys.argv[1], "r")
datafile = f.read()
f.close()

ledOn = 0
ledOff = 0
enableOn = 0
enableOff = 0

def formatTime(ms):
    hours = ms / (60 * 60 * 1000)
    minutes = (ms - hours * 60 * 60 * 1000) / (60 * 1000)
    seconds = (ms - (hours * 60 * 60 * 1000 + minutes * 60 * 1000)) / 1000
    millis = (ms - (hours * 60 * 60 * 1000 + minutes * 60 * 1000 + seconds * 1000))
    hourstr = ""
    minutestr = ""
    if hours > 0:
        hourstr = "%02d:" % hours
    if (hours > 0) or (minutes > 0): 
        minutestr = "%02d:" % minutes
        secondstr = "%02d." % seconds
    else:
        secondstr = "%d." % seconds
    millistr = "%03d" % millis
    return "%s%s%s%s" % (hourstr, minutestr, secondstr, millistr)

for l in datafile.split("\r\n"):
    if len(l) == 0 or l[0] == "#":
        continue
    (timestr, led, enable, change) = l.split('\t')
    time = int(timestr)
    if change == "L":        
        if led == "1":
            print "%s\t%s\t%s\tLED Off" % (formatTime(ledOff + baseTime), formatTime(time + baseTime), formatTime(time - ledOff))
            ledOn = time
        else:
            print "%s\t%s\t%s\tLED On" % (formatTime(ledOn + baseTime), formatTime(time + baseTime), formatTime(time - ledOn))
            ledOff = time
    elif change == "E":
        if enable == "1":
            print "%s\t%s\t%s\tENABLE Off" % (formatTime(enableOff + baseTime), formatTime(time + baseTime), formatTime(time - enableOff))
            enableOn = time
        else:
            print "%s\t%s\t%s\tENABLE On" % (formatTime(enableOn + baseTime), formatTime(time + baseTime), formatTime(time - enableOn))
            enableOff = time
    
