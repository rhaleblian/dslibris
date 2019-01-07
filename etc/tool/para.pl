#!/usr/bin/perl
# try to infer paragraphs in books
# with hard line breaks, eg from pdftohtml.

while(<>)
{
    s|(\.)\s*<br>|$1<p>|g;
    s|(\!)\s*<br>|$1<p>|g;
    s|(\?)\s*<br>|$1<p>|g;
    s|(\")\s*<br>|$1<p>|g;
    s|(\&quot;)\s*<br>|$1<p>|g;
    s|<br>| |g;
    s|<hr>||g;
    print;
}
