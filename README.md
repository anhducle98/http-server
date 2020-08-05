# Basic HTTP server project description

This is a very basic HTTP server.

### Features:
* Support HTTP GET method
* Serve static files
* Support persistent connections
* Asynchronous request handling
* Multiple worker threads

### Build
A `Makefile` is provided, simply run `$ make` in the project directory.

### Usage
```
$ ./http-server -h
usage: http-server [-p <port>] [-f <root dir>] [-n <num workers>]
```

* `port`: which port to listen on, default `8080`
* `root dir`: which directory to serve, default `www` containing a simple HTML page for demonstration
* `num workers`: number of worker threads, default `2`

### Benchmark
Environment: `ThinkPad T450s` with `Intel® Core™ i7-5600U CPU @ 2.60GHz`.

##### Run server
```
$ ./http-server -n 3
root dir = "www"
port = 8080
backlog = 4096
num_workers = 3
Server started, listening on port 8080
```
##### Load test
```
$ wrk -c256 -d10 -t2  http://127.0.0.1:8080/test.txt
Running 10s test @ http://127.0.0.1:8080/test.txt
  2 threads and 256 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    11.12ms   14.25ms  63.35ms   79.10%
    Req/Sec    40.28k     3.96k   47.62k    79.00%
  801650 requests in 10.08s, 113.94MB read
Requests/sec:  79515.19
Transfer/sec:     11.30MB
```
```
$ wrk -c10000 -d10 -t2  http://127.0.0.1:8080/test.txt
Running 10s test @ http://127.0.0.1:8080/test.txt
  2 threads and 10000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   117.38ms   42.37ms 311.27ms   66.76%
    Req/Sec    35.00k     4.50k   50.51k    71.76%
  675077 requests in 10.01s, 96.19MB read
Requests/sec:  67415.22
Transfer/sec:      9.61MB
```

### Technical details
The main thread continuously listens for new connections and delegates processing to worker threads in a round-robin manner, uses C++11's `std::thread` for simplicity.

Spawn `num_workers` independent threads for handling request. Each actively grab ready TCP sockets using `epoll` and handle them accordingly.

When a response has been sent, if the request contains `Connection: Keep-Alive`, keep the connection for future requests, else close it.
