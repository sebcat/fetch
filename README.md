# fetch

Playing around with the libcurl multi API

````
$ ./fetch -h
Read a list of urls from stdin, print results to stdout
args:
  n <1-100>           Concurrent transfer limit
  r <host:port:addr>  Resolve host:port to addr
````

1 vs. 8 concurrent transfers:

````
$ time ./fetch -n1 < urllist.txt
http://www.google.com/f (404) 1767B
http://www.google.com/a (301) 597B
http://www.google.com/z (404) 1767B
http://www.google.com/ (302) 484B
http://lolwut/ (-1) 0B
http://www.google.se/f (404) 1767B
http://www.google.se/a (301) 612B
http://www.google.se/z (404) 1767B
./fetch -n1 < urllist.txt  0,01s user 0,00s system 1% cpu 0,600 total
$ time ./fetch -n8 < urllist.txt
http://lolwut/ (-1) 0B
http://www.google.com/ (302) 488B
http://www.google.se/a (301) 610B
http://www.google.com/a (301) 597B
http://www.google.se/f (404) 1767B
http://www.google.se/z (404) 1767B
http://www.google.com/z (404) 1767B
http://www.google.com/f (404) 1767B
./fetch -n8 < urllist.txt  0,00s user 0,01s system 6% cpu 0,218 total
````

Resolve URL host:port tuples to specific addresses and set the Host header accordingly:

````
./fetch -n8 -r 'lolwut:80:2a00:1450:400f:802::1005' < urllist.txt
http://www.google.com/a (301) 593B
http://www.google.com/f (404) 1767B
http://www.google.com/z (404) 1767B
http://www.google.se/a (301) 607B
http://www.google.se/f (404) 1767B
http://www.google.se/z (404) 1767B
http://www.google.com/ (200) 54151B
http://lolwut/ (302) 477B
````
