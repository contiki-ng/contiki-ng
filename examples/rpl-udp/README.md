A simple RPL network with UDP communication. The DAG root also acts as
UDP server. All other nodes are client. The clients send a UDP request
that simply includes a counter as payload. When receiving a request, The
server sends a reply with the same counter back to the originator.

The simulation files show example networks, for sky motes and for cooja motes.
