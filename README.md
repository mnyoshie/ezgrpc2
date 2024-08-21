# EZgRPC2

A single threaded, non-blocking, asynchronous, gRPC library in C.

This library doesn't necessarily makes the implementation of gRPC server easier, in fact,
it makes it harder.

This library does not provide a way to serialized message and is not dependent on any serialization
library such as protobuf or flatbuffers. You would would have to manually write your own to
serialize, deserialize your own messages.

## SSL/TLS support

To be determine.

# Architecture

This arhitecture was inspired by `poll(2)`, but instead of polling fds and returning events
such as POLLIN, you poll a lists of paths and it gives events of `EVENT_MESSAGE`,
`EVENT_DATALOSS` and `EVENT_CANCEL` to specific stream ids.

Pseudocode:


``` 
int main() {
  int res;
  const size_t nb_paths = 2;
  ezgrpc2_path_t paths[nb_paths];
  struct path_userdata_t path_userdata[nb_paths];
#ifdef __unix__
  /* In a real application, user must configure the server
   * to handle SIGTERM, and make sure to prevent these
   * signal from propagating through the main threads and
   * pool threads via pthread_sigmask(SIG_BLOCK, ...) for
   * the signal handler and pthread_sigmask(SIG_SETMASK, ...)
   * for the rest. This is skipped.
   */
  signal(SIGPIPE, SIG_IGN);
#endif


  /* Tasks for unary requests (single message with end stream) */
  pthpool_t *unordered_pool = NULL;
  /* Tasks for streaming requests (multiple messages in a stream).
   * streaming rpc is generally slower than unary request since
   * the messages must be ordered and hence can't be parallelize */
  pthpool_t *ordered_pool = NULL;

  unordered_pool = pthpool_init(2, -1);
  assert(unordered_pool != NULL);

  /* worker must be one for an ordered execution */
  ordered_pool = pthpool_init(1, -1);
  assert(ordered_pool != NULL);
  //ezgrpc2_server_t *server = ezgrpc2_server_init("127.0.0.1", 19009, NULL, -1, 16);
  ezgrpc2_server_t *server = ezgrpc2_server_init("0.0.0.0", 19009, "::", 19009, 16);
  assert(server != NULL);


  /*-----------------------------.
  | What services do we provide? |
  `-----------------------------*/

  paths[0].path = "/test.yourAPI/whatever_service1";
  paths[1].path = "/test.yourAPI/whatever_service2";
  list_init(&paths[0].list_events);
  list_init(&paths[1].list_events);
  /* we expect to receive one or more grpc message in a
   * single stream.
   * */
  path_userdata[0].is_unary = 0;
  path_userdata[0].callback = callback_path0;
  paths[0].userdata = path_userdata + 0;
  /* we expect to receive only one grpc message in a
   * single stream
   */
  path_userdata[1].is_unary = 1;
  path_userdata[1].callback = callback_path1;
  paths[1].userdata = path_userdata + 1;





  //  gRPC allows clients to make concurent requests to a server in
  //  a single connection by making use of stream ids in HTTP2.
  //  
  //  A gRPC request to a server, at it's bare minimum, must have a `path`
  //  and one or more `grpc message`s.
  //  
  //  When a client makes a requests, that request is not necessarily immediately
  //  executed, instead, the request is added to the queue in the thread pool,
  //  waiting to be executed among other requests.
  //  
  //  When the tasks has been executed, the result is added to the finished
  //  queue, waiting to be pulled off with, `pthpool_poll()`. After that
  //  we can then send our results.
  //  
  //  So basically, we have a loop of:
  //  
  //    1. Get server events. (server poll)
  //    2. Add tasks to the thread pool. (give the task to thread pool).
  //    3. Get finish tasks from the thread pool (thread pool poll)
  //    4. Send the results. (give the result to the client)


  while (1) {
    int is_pool_empty = pthpool_is_empty(unordered_pool) &&
                        pthpool_is_empty(ordered_pool);

    /* if thread pool is empty, maybe we can give our resources to the cpu
     * and wait a little longer.
     */
    int timeout = is_pool_empty ? 10000 : 10;

#ifdef __unix__
    // if sigterm flag has been set by the signal handler, break the loop and kill
    // server.

    /*   .... */
#endif
    // step 1. server poll
    if ((res = ezgrpc2_server_poll(server, paths, nb_paths, timeout)) < 0)
      break;

    if (res > 0) {
      // step 2. Give the task to the thread pool
      handle_events(server, paths, nb_paths, ordered_pool, unordered_pool);
    }
    else if (res == 0) {
      printf("no event\n");
      // No server events, let's check the thread pool.
    }
    // step 3, 4 Get finish tasks and send results
    handle_thread_pool(server, ordered_pool);
    handle_thread_pool(server, unordered_pool);
  }

  if (res < 0) {
    printf("poll err\n");
  }


  ezgrpc2_server_destroy(server);
  pthpool_destroy(ordered_pool);
  pthpool_destroy(unordered_pool);
  return res;
}

```

see `https://github.com/mnyoshie/ezgrpc2/blob/master/examples/hello_worldc.c` for a complete
MWE server.
