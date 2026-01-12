Defined custom ethertype:

```sh
ETH_P_NEIGHBOR_DISC = 0x88B5
```

TODO list:

1. CODE CLEANUP, check comments. rearrange project files + upd makefile

2. Think about if this will work fine with 10 000 neighbours, optimize stuff in code. check out select() and poll() calls. Think about cache limits

3. Format a readme file to describe this project

FINALLY:

4. Test this, create test scenarios. Is it fine with virtualbox?
5. final code review!!!! Reheck if everything matches criteria


CLI output example:
```
=== Neighbor 1 ===
Machine ID: a7b7821a7ab44b049fb879e2b201fdb0

  Connection 1:
    Local interface: enp0s9
    Remote MAC:      08:00:27:76:60:21
    Remote IPv4:     none
    Remote IPv6:     none

  Connection 2:
    Local interface: enp0s8
    Remote MAC:      08:00:27:ab:30:7e
    Remote IPv4:     none
    Remote IPv6:     none

Total neighbors: 1
```