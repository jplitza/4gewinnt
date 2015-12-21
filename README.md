4gewinnt
========

This simple C implementation of "connect four" features a client/server design
to play against each other or against an AI.

It was originally built as part of a coding contest.

![Screenshot](/screenshot.png?raw=true)

Dependencies
------------

`libncursesw` should be enough.

Help messages
-------------

### gui
```
Usage: gui [options]

  -c, --client             start in client mode
  -S, --server             start in server mode
  -l, --local              start in local mode (default)
  -a, --ai                 start in human vs. computer mode
  -s, --host               specify server to connect to (only for client mode)
                           (default: localhost)
  -p, --port               specify port to connect to or port to listen
                           (only in client and server mode, default: 6666)
  -h, --help               show this help
```

### ai
```
Usage: ai [options]

  -s, --server, --host <server>  specify server to connect to
                                 (default: localhost)
  -p, --port <port>              specify port to connect to
                                 (default: 6666)
  -o, --output                   print game grid after game
                                 (useful in AI vs AI games)
  -h, --help                     show this help
```

### server
```
Usage: server [port]
```
