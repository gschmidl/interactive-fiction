#!/usr/bin/env python
# Advent Magic Mode authentication
# Based on (read: ripped off) Zona de pruebas
# <http://www.zonadepruebas.com/magicmode_crack_en.html>

from __future__ import print_function
import sys
import getopt
import math
import re
import time

def usage():
	print("Usage: %s [-m magic] [-t time] challenge" % sys.argv[0])
	print("")
	print("\t-m magic    Magic number (default 11111)")
	print("\t-t time     Time in HH:MM format (default now)")

def calc(ch, ts, d):
	if len(ch) == 5 and re.match(r"^[a-z]{5}$", ch, re.I):
		ch = map(ord, ch.upper())
	else:
		print("Challenge must be 5 letters", file=sys.stderr)
		return None
	
	try:
		d = int(d)
	except ValueError:
		print("Magic must be an integer", file=sys.stderr)
		return None

	if ts is None:
		ts = time.localtime()
	else:
		try:
			ts = time.strptime(ts, "%H:%M")
		except ValueError:
			print("Invalid time (must be in HH:MM format)",
				file=sys.stderr)
			return None
	t = ts.tm_hour*100 + math.floor(ts.tm_min/10)*10

	s = ""
	for y in range(5):
		z = (y+1)%5
		x = int(((abs(ch[y]-ch[z])*(d%10))+(t%10))%26+1)
		s += chr(x+64)
		t = math.floor(t/10)
		d = math.floor(d/10)
	
	return s

if __name__ == "__main__":
	ch = None
	ts = None
	magic = "11111"

	try:
		opts, args = getopt.getopt(sys.argv[1:], "m:t:")
	except getopt.error as e:
		print("%s: %s" % (sys.argv[0], e), file=sys.stderr)
		usage()
		sys.exit(2)

	for opt, optarg in opts:
		if opt == "-m":
			magic = optarg
		elif opt == "-t":
			ts = optarg

	try:
		ch = args[0]
	except IndexError:
		print("Challenge not specified", file=sys.stderr)
		usage()
		sys.exit(2)

	res = calc(ch, ts, magic)
	if res is None:
		sys.exit(1)
	else:
		print(res)
