Defined custom ethertype: 

```c
ETH_P_NEIGHBOR_DISC = 0x88B5
```

Stuff TODO:

1. Get interface ip4 and/or ip6 instead of dummy dumb data.
2. Currently data saved in map (where MAC is unique key), need to check if entries have timeouted (last_seen > 30s)
3. What to do with neighbours, that are connected via multiple ethernet interfaces? and if each interface has its own NIC MAC address, is this still 1 neigbour?
4. Current stuff needs to work on multiple interfaces - currently 1 is parsed ar cmnd line argument. Dynamically adapt to appeared/disappeared/changed-state interfaces.
5. Think about if this will work fine with 10 000 neighbours
6. still a lot of stuff to do