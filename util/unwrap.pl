#!/bin/sh
perl -pne 's/([\."\!])<br \/>/$1<\/p><p>/g; s/<br \/>//g' $*

