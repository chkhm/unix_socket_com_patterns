# unix_socket_com_patterns

Experimenting with Client Server and Point to Point connections via Unix sockets.

## What are the two applications doing

The two little executables can be used to try out unix domain socket-based client-server communication.
The server.cpp opens a unix domain socket and listens to incoming connect requests using `poll`.

This happens in a forever loop.

This way, at least theoretically, it should be able to handle multiple incoming connection requests.
When a request occurs it accepts it, and also starts monitoring it via `poll` for incoming messages. 
The incoming message is sent back to the client.

The client.cpp opens a unix domain socket and connects to the server socket - which leads to the above 
mentioned *accept* by the server. After the connect was successful, the client writes a message to the 
server (in fact it sends the first command line argument) and waits for the response.

When the server sends the message back the client prints it to sdtdout.

The client repeats this 10 times with a time interval of 5 seconds and then closes.

## Strange effect

On a Mac this does not work as expected. When I start a server and then afterwards two clients, the second client
is blocked on the write call until the first client has disconnected.

Apparently, the unix socket is serializing the access.

I haven't tried yet on a Linux machine.

## How to run the experiment

```text
terminal 1:                terminal 2:                 terminal 3:

$> server                  $> client aaa               $> client bbb

```

You will notice that `client aaa` will execute all the way through as expected but `client bbb` is stuck at the 
write command until the `client aaa` completes. On the server side the log message about accepting `client bbb`
will only show up after the `client aaa` completes. In other words, unix domain sockets seem to serialize access
to the server.


