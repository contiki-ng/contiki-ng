#!/bin/bash
#
# End-to-end test of the ip64 NAT64 service against real host software, over a
# Linux TAP device. The node runs ip64 with its IPv4 side on a TAP device that
# it creates itself, gives the host end 192.0.2.1, and answers to 192.0.2.50,
# so that programs on the host reach it over UDP, ICMP and DNS without knowing
# anything about NAT64.
#
# The suite Makefile builds the node twice: with the address compiled in, and
# with IP64_CONF_DHCP=1, which takes it from a DHCP server on the host as the
# Orion border router does. This script runs whichever build is there and
# reads from the node's own IP64_TAP_MODE line which one that is.
#
# Creating the TAP device needs CAP_NET_ADMIN. CI runs this in a privileged
# container, but as an ordinary user with passwordless sudo, so take that
# route when it is there rather than refusing to run.

if [ "$(id -u)" -ne 0 ]; then
  if sudo -n true 2>/dev/null; then
    echo "Not running as root; continuing under sudo"
    exec sudo -E "$0" "$@"
  fi

  if [ "${IP64_TAP_ALLOW_SKIP:-0}" = "1" ]; then
    echo "SKIP: no way to get CAP_NET_ADMIN, and IP64_TAP_ALLOW_SKIP=1 is set"
    exit 0
  fi

  echo "TEST FAIL: this test needs CAP_NET_ADMIN to create the TAP device."
  echo "           Run it as root, arrange passwordless sudo, or set"
  echo "           IP64_TAP_ALLOW_SKIP=1 to skip it."
  exit 1
fi

# With the capability in hand, the device node still has to be there. In a
# container that means the host's /dev, which is what --privileged gives.
if [ ! -c /dev/net/tun ]; then
  echo "TEST FAIL: /dev/net/tun is missing, so no TAP device can be created."
  echo "           A container needs --privileged, or at least this device."
  exit 1
fi

source ../utils.sh

# The echo server is the one the Cooja NAT64 test uses, so that both tests
# talk to the same IPv4 program.
ECHO_SERVER=$(cd ../17-tun-rpl-br && pwd)/nat64-echo-server.py

BASENAME=27-ip64-tap
cd $BASENAME || exit 1

NODE=./build/native/ip64-tap-node.native
NODE_IPV4=192.0.2.50
HOST_IPV4=192.0.2.1
TAP_DEV="${IP64_TAP_DEV:-tap0}"
UDP_PORT=5557
TCP_PORT=5558
DNS_PORT=53
# The answer differs from the DNS server's own address, so the address the
# node reports can only have come from the A record.
LOOKUP_NAME=ip64-test.example
LOOKUP_ANSWER=192.0.2.99
LOOKUP_EXPECTED="64:ff9b::192.0.2.99"

NODE_LOG=$BASENAME.node.log
ECHO_LOG=$BASENAME.echo.log
DNS_LOG=$BASENAME.dns.log
DHCP_LOG=$BASENAME.dhcp.log
PING_LOG=$BASENAME.ping.log

test_init

echo "-- Starting test $BASENAME"

register_logfile $NODE_LOG
$NODE &> $NODE_LOG &
register_last_bg_cmd

# The TAP device disappears with the node, so everything that binds the host
# address has to start after this.
wait_log_assert "node creates the TAP device" "IP64_TAP_DEVICE_UP" \
                $NODE_LOG 30

if grep -q "IP64_TAP_MODE dhcp" $NODE_LOG 2>/dev/null; then
  echo "-- The node takes its address from DHCP"

  # Bound to the TAP device, so serving 0.0.0.0:67, which is what receiving a
  # broadcast takes, cannot disturb the rest of the machine. The lease travels
  # through ip64: the client sends to 64:ff9b::255.255.255.255.
  register_logfile $DHCP_LOG
  python3 ./dhcp-server.py \
    --device "$TAP_DEV" --offer $NODE_IPV4 --server $HOST_IPV4 \
    --router $HOST_IPV4 --log $DHCP_LOG &
  register_last_bg_cmd

  wait_log_assert "DHCP server starts" "DHCP_LISTEN" $DHCP_LOG 10
  wait_log_assert "DHCP server leases $NODE_IPV4" "DHCP_ACK ip=$NODE_IPV4" \
                  $DHCP_LOG 30
  wait_log_assert "node configures itself from the lease" \
                  "IP64_TAP_LEASE $NODE_IPV4 router $HOST_IPV4" $NODE_LOG 30
fi

wait_log_assert "node is ready" "IP64_TAP_READY" $NODE_LOG 30

# Both servers bind the TAP address rather than every address, so that they do
# not collide with a resolver or another service already on this machine. The
# node retransmits, so requests that go nowhere until they are up cost nothing.
register_logfile $ECHO_LOG
python3 "$ECHO_SERVER" \
  --host $HOST_IPV4 --udp-port $UDP_PORT --tcp-port $TCP_PORT \
  --log $ECHO_LOG &
register_last_bg_cmd

wait_log_assert "echo server starts" "UDP_LISTEN" $ECHO_LOG 10

register_logfile $DNS_LOG
python3 ./dns-server.py \
  --host $HOST_IPV4 --port $DNS_PORT --name $LOOKUP_NAME \
  --answer $LOOKUP_ANSWER --log $DNS_LOG &
register_last_bg_cmd

wait_log_assert "DNS server starts" "DNS_LISTEN" $DNS_LOG 10

# The IPv6->IPv4 leg: an IPv4 receiver that knows nothing about NAT64 sees the
# node's datagram arrive from the translated address.
wait_log_assert "echo server receives IPv4 from $NODE_IPV4" \
                "UDP_ECHO from=$NODE_IPV4:.*payload=b'PING-IP64-TAP'" \
                $ECHO_LOG 30

# The return leg: the reply comes back translated, on a datagram this test did
# not build.
wait_log_assert "node receives the reflected datagram" "IP64_TAP_ECHO_OK" \
                $NODE_LOG 30

# The node asks for AAAA (resolv.c queries NATIVE_DNS_TYPE), so an A query
# arriving at an IPv4-only server is the DNS64 rewrite doing its work, and the
# address the node ends up with is the A record behind the NAT64 prefix.
wait_log_assert "DNS server receives an A query" \
                "DNS_QUERY .*name=$LOOKUP_NAME qtype=1" $DNS_LOG 30
wait_log_assert "node resolves $LOOKUP_NAME" \
                "IP64_TAP_DNS_OK $LOOKUP_NAME $LOOKUP_EXPECTED" $NODE_LOG 30

# Inbound ICMP translation, and ip64 answering the host's ARP request.
register_logfile $PING_LOG
assert "host pings the node at $NODE_IPV4" \
       "ping -c 3 -W 2 $NODE_IPV4 > $PING_LOG 2>&1"

do_wrap_up
