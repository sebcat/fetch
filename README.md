# curl-multi

Learning how to use the libcurl multi API

````
$ ./curl-multi -h
Read a list of urls from stdin, print results to stdout
args:
  n        maximum number of concurrent transfers (1-100)
$ time ./curl-multi -n1 < urllist.txt
http://www.google.com/f (404) 1767B
http://www.google.com/a (301) 597B
http://www.google.com/z (404) 1767B
http://www.google.com/ (302) 484B
http://lolwut/ (-1) 0B
http://www.google.se/f (404) 1767B
http://www.google.se/a (301) 612B
http://www.google.se/z (404) 1767B
./curl-multi -n1 < urllist.txt  0,01s user 0,00s system 1% cpu 0,600 total
$ time ./curl-multi -n8 < urllist.txt
http://lolwut/ (-1) 0B
http://www.google.com/ (302) 488B
http://www.google.se/a (301) 610B
http://www.google.com/a (301) 597B
http://www.google.se/f (404) 1767B
http://www.google.se/z (404) 1767B
http://www.google.com/z (404) 1767B
http://www.google.com/f (404) 1767B
./curl-multi -n8 < urllist.txt  0,00s user 0,01s system 6% cpu 0,218 total
````
