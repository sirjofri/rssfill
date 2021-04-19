rssfill
=======

This program reads a rss/atom feed on stdin and puts their items
in files in a specified directory.

	hget https://server/path/to/rss-feed.xml | rssfill -ct -p prefix -d directory

Arguments:

* `-c`: chatty on. Tell me everything.
* `-t`: don't do anything. Parse stuff, but don't place files.
* `-p prefix`: prefix the filenames with the given prefix.
* `-d directory`: put files into this directory.

For best integration, you can run this directly on `/lib/news`, the
default `news(1)` directory, or use `bind(1)` to build your own
`/lib/news`. Make a loop with a `sleep` command to automatically
fetch data every hour or use a cronjob.

Building and Installation
-------------------------

This package includes an updated version of `xmlpull` from contrib.

	mk install
