
# Neighbor Discovery Service

A background service that discovers neighbors running the same service in connected Ethernet networks.

Defined custom ethertype:

```sh
ETH_P_NEIGHBOR_DISC = 0x88B5
```

TODO list:

1. Think about if this will work fine with 10 000 neighbours, optimize stuff in code. check out select() and poll() calls. Think about cache limits
optimize main loop

2. Cleanup to do stuff:
  * check comments, delete garbage
  * avout include paths like "../.." (update makefile for this?)
  * logging outputs even when running as &, should cout be directed somewhere? or logging should be ditched, just use perror()

3. Format a readme file to describe this project

FINALLY:

4. Test this, create test scenarios. Is it fine with virtualbox?
5. final code review!!!! Reheck if everything matches criteria

## Running the Service
### Start in background with logging:
```bash
sudo ./neighbor_discovery_service >> service.log 2>&1 &
```

### View logs:
```bash
tail -f service.log
```

### Stop service:
```bash
sudo pkill neighbor_discovery_service
```


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