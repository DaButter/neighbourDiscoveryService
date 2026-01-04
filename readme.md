Defined custom ethertype:

```c
ETH_P_NEIGHBOR_DISC = 0x88B5
```

TODO list:

1. Current stuff needs to work on multiple interfaces - currently 1 is parsed as cmnd line argument. Dynamically adapt to appeared/disappeared/changed-state interfaces.
2. What to do with neighbours, that are connected via multiple ethernet interfaces? and if each interface has its own NIC MAC address, is this still 1 neigbour?
3. Think about if this will work fine with 10 000 neighbours, optimize stuff in code. check out select() and poll() calls. Think about cache limits
4. If interface has multiple ipv4 and ipv6 addresses, need to upgrade the protocol... quite messy stuff.
5. Implement CLI program (somesort of unix socket will be needed, will think about it later)

FINALLY:
6. Test this, create test scenarios. Is it fine with virtualbox?
7. Code review!!!! Check if everything matches criteria