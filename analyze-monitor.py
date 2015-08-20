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

ledStartPwmTime = None
ledLastTime = None
ledLastValue = None
enableStartPwmTime = None
enableLastTime = None

maxPwmTime = 18

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
        if ledLastTime is not None:
            if ((time - ledLastTime) <= maxPwmTime):
                ledLastTime = time
                if ledStartPwmTime is None:
                    ledStartPwmTime = ledLastTime
            else:
                if ledStartPwmTime is not None:
                    print "%s\t%s\t%s\t%s\tLED PWM" % (formatTime(ledStartPwmTime + baseTime),
                                                       formatTime(ledLastTime + baseTime),
                                                       formatTime(ledLastTime - ledStartPwmTime),
                                                       timestr)
                    ledStartPwmTime = None
                if led == "1":
                    print "%s\t%s\t%s\t%s\tLED Off %s" % (formatTime(ledLastTime + baseTime), formatTime(time + baseTime), formatTime(time - ledLastTime), timestr, ledLastValue)
                else:
                    print "%s\t%s\t%s\t%s\tLED On  %s" % (formatTime(ledLastTime + baseTime), formatTime(time + baseTime), formatTime(time - ledLastTime), timestr, ledLastValue)
        ledLastTime = time                
        ledLastValue = led
