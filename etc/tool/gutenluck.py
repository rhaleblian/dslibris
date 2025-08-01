#!/usr/bin/python
##! Convert a file or URL of a PG ASCII file to XHTML.
##  Requires GutenMark and HTML Tidy.
##  usage: gutenluck.py <url> [outfile]
##
## <site-configuration>
path_gut_program_root = "/Applications/GUItenMark.app/Contents/MacOS/"
path_gut_config_root = "/Applications/GUItenMark.app/Contents/Resources/GutenMark.cfg"
path_tidy_program_root = ""
## </site-configuration>

import os, sys, urllib
from subprocess import *

infile = sys.argv[1]
outfile = '-'
if len(sys.argv) > 2:
    outfile = sys.argv[2]

# For local files, infile remains unchanged. Nice.
(infile,headers) = urllib.urlretrieve(infile)
print('infile: ' + infile)

p1 = Popen([path_gut_program_root + "GutenMark",
            "--config=" + path_gut_config_root,
            infile],
           stdout=PIPE)

tidy_cmd = [path_tidy_program_root + 'tidy',            
            '-asxhtml',
            '-numeric']
if outfile != '-':
    tidy_cmd.append("-o")
    tidy_cmd.append(outfile)

p2 = Popen(tidy_cmd,stdin=p1.stdout)
p2.communicate()[0]
