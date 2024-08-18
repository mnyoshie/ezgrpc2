# EZgRPC2

A single threaded, non-blocking, asynchronous, gRPC library in C.

This library doesn't necessarily makes the
implementation of gRPC servers/clients easier, in fact,
it makes it harder.

# Architecture

This arhitecture was inspired `poll(2)`. The idea is simple, we poll our
server for events, and process their events in a non-blocking way, like
for example, poll, update, render, repeat.

