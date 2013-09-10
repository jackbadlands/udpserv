The project have the following parts:

* udpserv - listen UDP port, execute command for each packet
* udpclient - sends repeated UDP packets
* auth-and-execute - script for usage with udpserv, provides 
authentication, encryption and deduplication for allowing 
execution of arbitrty commands over udpserv
* auth-and-execute-client - tool to send requests to udpserv 
running auth-and-execute
* mosh-connect - tool to start mosh session across udpserv/auth-
and-execute instead of SSH

The whole system is designed to reach across high packet loss (up 
to 90%) network, for example, to server under DoS.


udpserv
---
```
Usage:
	udpserv delay_after_each_request_microsec recv_host recv_port cmdline...
Example:
	udpserv 25000 0.0.0.0 6767 sh -c 'read I; ifconfig "$I"' 
	echo eth0 | nc -u 127.0.0.1 5555 -q 1

	./udpserv 200000 0.0.0.0 2222 ./auth-and-execute 0x09EA92A2DB6D6082 0x09EA92A2DB6D6082 /bin/bash

```

`udpserv` opens UDP socket and for each incoming packet executes 
specified command line and sends its output back to client, 
somewhat like `socat udp-l:2222,fork exec:'command line'`


udpclient
---
```
Usage:
	udpclient count interval send_host send_port < input > output
Example:
	echo eth0 | udpserv 10 25000   1.2.3.4 6767 

```
`udpclient` reads full input (until EOF), sends it to UDP socket 
several times (until some reply comes) and outputs the reply UDP 
packet to stdout. Somewhat like `nc -u 1.2.3.4 6767`, but not 
line-oriented and repeats packets automatically. 


auth-and-execute
---
```
Usage: ./udpserv interval liten_address port ./auth-and-execute 0xGPG_client_keyid 0xGPG_server_keyid shell command line
Server GPG key should be available without passphrase
Example: ./udpserv 200000 0.0.0.0 2222 ./auth-and-execute 0x09EA92A2DB6D6082 0x09EA92A2DB6D6082 /bin/bash
```
`auth-and-execute` is started by udpserv for each incoming packet. 
It decrypts incoming packet using GPG, checks the signature and, 
if signature is correct, executes the command, encrypting and 
sending the reply. It also caches replies to interact well with 
retransmissions.

You should manually set up GPG keys on server and client to use 
auth-and-execute. You must specify full long 0x-prefixed key IDs 
(`gpg --keyid-format=0xlong --list-keys`).

auth-and-execute-client
---
```
Usage: ./auth-and-execute-client gpg_user_key gpg_server_key host port     command
Example: ./auth-and-execute-client 0x09EA92A2DB6D6082 0x09EA92A2DB6D6082 1.2.3.4 2222   ping -c 5 -i 0.2 8.8.8.8
```
`auth-and-execute-client` is client for auth-and-execute.

It appends anti-replay-attack (don't rely on it too much although) 
fragment to your scripts (based on UTC time on client and server) 
and sends the request using `udpclient`. Use it as "poor man's 
SSH" in case of usual SSH in unreachable due to poor network. 
All executed commands are expected to be short (fit in one UDP 
packet, including gpg overhead), their output should also be short 
and they should not run for long (the client will retransmit 
requests again and again, stacking up pending to-be-served-from-
cache requests on server).


mosh-connect
---
```
Usage: mosh-connect gpg_user_key gpg_server_key host port [mosh-server command line]
Example: mosh-connect 0x09EA92A2DB6D6082 0x09EA92A2DB6D6082 1.2.3.4 2222 LANG=en_US.UTF-8 /root/bin/mosh-server
Only new enough (with commit 45bba44, > 1.2.4) mosh-server can probably be used with this script
```

Uses `auth-and-execute-client` to set up [mosh](https://github.com/keithw/mosh) session.
