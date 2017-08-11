#!/usr/bin/env python
import re
import sys
import random as R

inc = None
over = None
count = 50
frame = [0,0,300,200]
if len(sys.argv) > 1:
	frame = map(int,sys.argv[1].split(','))
if len(sys.argv) > 2:
	try:
		count = int(sys.argv[2])
	except ValueError:
		m = re.match(r'^(\d+)/(\d+)$',sys.argv[2])
		inc = int(m.group(1))
		over = int(m.group(2))

def rand(a,b,count=2):
	n = R.randint(a,b)
	if count <= 1:
		return (n % (b-a)) + a
	return (n + rand(a,b,count-1)) % (b-a)

if inc and over:
	print "x,y,z"
	for y in xrange(frame[1],frame[3]):
		v = y * inc
		for x in xrange(frame[0],frame[2]):
			v = max(x,y) % over
			print ','.join(map(str,(x,y,v)))
			v += inc
else:
	print "x,y"
	print "\n".join(
		(str(x)+','+str(y) for x,y in
  	  ((rand(frame[0],frame[2]),rand(frame[1],frame[3]))
			for i in xrange(count)))
	)

