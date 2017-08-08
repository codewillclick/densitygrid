#!/usr/bin/env python
import sys
import random as R

count = 50
frame = [0,0,300,200]
if len(sys.argv) > 1:
	frame = map(int,sys.argv[1].split(','))
if len(sys.argv) > 2:
	count = int(sys.argv[2])
def rand(a,b,count=2):
	n = R.randint(a,b)
	if count <= 1:
		return (n % (b-a)) + a
	return (n + rand(a,b,count-1)) % (b-a)
print "x,y"
print "\n".join(
	(str(x)+','+str(y) for x,y in
    ((rand(frame[0],frame[2]),rand(frame[1],frame[3]))
		for i in xrange(count)))
)

