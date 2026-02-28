Read through the headers in m-karma/include/ so you can understand the current public API.
  Read everything in m-karma/src/cli/ and m-karma/src/log/ and m-karma/demo/cli/install/src/ so you can understand the
  fundamentals of the cli bootstrap process as they exist today.
  Run a couple --trace commands from m-karma/demo/cli/install/build/{client|server} to see that they do what you expect.
  We are in the process of greatly reducing the footprint of our public API. If you look at include/karma/log/trace.hpp,
  that is an example of where we want to get. Before a refactor, this file also included everything in src/log/private.hpp.
  We are going to start going through all of the public headers and stripping them down to what is actually intended for
  public consumption.
  Please return when you have completed these tasks and are ready to start.
