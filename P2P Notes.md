# Taken directly from [zguide.zeromq.org](https://zguide.zeromq.org/docs/chapter8/#True-Peer-Connectivity-Harmony-Pattern)

We want a guarantee that when a peer “appears” on our network, we can talk to it safely without ZeroMQ dropping messages. For this, we have to use a DEALER or PUSH socket that connects out to the peer so that even if that connection takes some non-zero time, there is immediately a pipe and ZeroMQ will accept outgoing messages.

A DEALER socket cannot address multiple peers individually. But if we have one DEALER per peer, and we connect that DEALER to the peer, we can safely send messages to a peer as soon as we’ve connected to it.

Now, the next problem is to know who sent us a particular message. We need a reply address that is the UUID of the node who sent any given message. DEALER can’t do this unless we prefix every single message with that 16-byte UUID, which would be wasteful. ROUTER does do it if we set the identity properly before connecting to the router.

And so the Harmony pattern comes down to these components:

- One ROUTER socket that we bind to a ephemeral port, which we broadcast in our beacons.
- One DEALER socket per peer that we connect to the peer’s ROUTER socket.
- Reading from our ROUTER socket.
- Writing to the peer’s DEALER socket.

The next problem is that discovery isn’t neatly synchronized. We can get the first beacon from a peer after we start to receive messages from it. A message comes in on the ROUTER socket and has a nice UUID attached to it, but no physical IP address and port. We have to force discovery over TCP. To do this, our first command to any new peer to which we connect is an OHAI command with our IP address and port. This ensure that the receiver connects back to us before trying to send us any command.

Here it is, broken down into steps:

- If we receive a UDP beacon from a new peer, we connect to the peer through a DEALER socket.
- We read messages from our ROUTER socket, and each message comes with the UUID of the sender.
- If it’s an OHAI message, we connect back to that peer if not already connected to it.
- If it’s any other message, we must already be connected to the peer (a good place for an assertion).
- We send messages to each peer using the per-peer DEALER socket, which must be connected.
- When we connect to a peer, we also tell our application that the peer exists.
- Every time we get a message from a peer, we treat that as a heartbeat (it’s alive).

If we were not using UDP but some other discovery mechanism, I’d still use the Harmony pattern for a true peer network: one ROUTER for input from all peers, and one DEALER per peer for output. Bind the ROUTER, connect the DEALER, and start each conversation with an OHAI equivalent that provides the return IP address and port. You would need some external mechanism to bootstrap each connection.