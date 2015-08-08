Trinket/GPS test for straightedge -- depends on all electrical
connections.

On power-up, it should do:

* 1 heartbeat with no GPS power (enable off)

* 1 heartbeat with yes GPS power (enable on)

* After GPS gets a fix (green GPS light blinks) -- two blinks per
second from the GPS echoed on the LED pin

Tests all four GPS-to-trinket connections:

No LED pin blinks until a valid GPRMC NMEA message (i.e., GPS position
fix) is received. It only receives a fix if the GPS->trinket serial
works.

No LED pin blinks if the pulse-to-pulse interval is <400 ms or >600
ms. Default pulse-to-pulse interval is 1 s (= 1000 ms) and it only
changes to 500 ms if the trinket->GPS serial works

No LED pin blinks if no pulses are received at all. Pulses are
received only if the GPS->trinket PPS signal works.

Switching the GPS off and then back on during start-up depends on a
working enable connection.
