#! /usr/bin/python
# remove cruft from HTML3 saved by Acrobat 7.
import re
import sys

infile = sys.argv[1]
fp = open(infile,'r')
buffer = fp.read()
p = re.compile('<BR>')
buffer = p.sub(' ',buffer)
p = re.compile('</?SPAN[^>]*>')
buffer = p.sub(' ',buffer)
print(buffer)
fp.close()

