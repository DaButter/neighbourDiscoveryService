Defined custom ethertype:

```sh
ETH_P_NEIGHBOR_DISC = 0x88B5
```

TODO list:

1. If interface has multiple ipv4 and ipv6 addresses, need to upgrade the protocol... quite messy stuff.

2. Think about if this will work fine with 10 000 neighbours, optimize stuff in code. check out select() and poll() calls. Think about cache limits

3. Implement CLI program (probably unix socket will be needed, will think about it later, a lot of stuff still to do)


FINALLY:

5. Test this, create test scenarios. Is it fine with virtualbox?
6. Code review!!!! Check if everything matches criteria