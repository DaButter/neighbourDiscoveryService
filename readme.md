Defined custom ethertype:

```sh
ETH_P_NEIGHBOR_DISC = 0x88B5
```

TODO list:

1. Recheck send and interface monitor timing in main.cpp. EthInterface and ActiveEthInterface - maybe a bit messy, recheck
2. Group functions in classes, for cleaner code, starting to get messy.
3. In updateInterface() take into consideration that mac address can also change for interface

4. What to do with neighbours, that are connected via multiple ethernet interfaces? and if each interface has its own MAC address, is this still 1 neigbour?
5. If interface has multiple ipv4 and ipv6 addresses, need to upgrade the protocol... quite messy stuff.
6. Think about if this will work fine with 10 000 neighbours, optimize stuff in code. check out select() and poll() calls. Think about cache limits

7. Implement CLI program (probably unix socket will be needed, will think about it later, a lot of stuff still to do)


FINALLY:

8. Test this, create test scenarios. Is it fine with virtualbox?
9. Code review!!!! Check if everything matches criteria