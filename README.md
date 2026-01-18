
# Neighbor Discovery Service

A background service that discovers neighbors running the same service in connected Ethernet networks.

## Overview

The service broadcasts discovery packets (L2 Ethernet frames) on all active Ethernet interfaces and listens for broadcasts from neighbors. Service maintains an active neighbor list with connection details (MAC, IPv4, IPv6) and provides a CLI tool to query the current state.

Full requirements defined in [requirements.md](./requirements.md).

## Architecture

### Components

1. **Background Service** (`ndisc_svc`)
   - Broadcasts discovery frames every 5 seconds (`SEND_INTERVAL_SEC` defined in `common.hpp`)
   - Receives and processes neighbor broadcasts
   - Maintains active neighbor list (using `std::unordered_map` of structs)
   - Provides IPC server for queries (Unix domain socket)

2. **CLI Tool** (`ndisc_cli`)
   - Queries service via Unix socket
   - Displays all active neighbors and connections
   - Shows interface names, MAC addresses, and IP addresses

### Protocol

**Ethertype**: `0x88B5` [IEEE 802 Local Experimental](https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml)


**Ethernet frame structure**:
```
+------------------+--------+  Header
| Destination MAC  | 6 B    |
| Source MAC       | 6 B    |
| EtherType        | 2 B    |
+------------------+--------+  Payload
| Machine ID       | 32 B   |
| IPv4 Address     | 4  B   |
| IPv6 Address     | 16 B   |
+------------------+--------+
Total: 66 bytes (14-byte Ethernet header + 52-byte payload).
```

Machine ID is read from `/etc/machine-id` (Ubuntu standard).


## Project Structure

- `src/main.cpp` — service entrypoint; main `select()` loop; send/receive scheduling
- `src/utils/interfaces.*` — interface discovery and raw socket setup per interface
- `src/utils/utils.*` — timestamp/log helpers, printing helpers
- `src/neighbor/neighbors.*` — neighbor/connection storage and timeout cleanup
- `src/ipc/server.*` — Unix domain socket IPC server for CLI queries
- `src/cli.cpp` — CLI client that queries IPC server and prints neighbors

## Building

Requires g++ with C++17 support:

```bash
make
```

This produces 2 executables:
- `ndisc_svc` - background service
- `ndisc_cli` - query tool


## Running
### Start Service

Must run as root for raw socket access:

```bash
sudo ./ndisc_svc
```

Or run in background with logging:

```bash
sudo -b ./ndisc_svc >> service.log 2>&1
```

### Query Neighbors

The CLI connects to a Unix domain socket at `/tmp/neighbor_discovery.sock`. Service creates the socket as root with restrictive permissions, `sudo` is needed:

```bash
sudo ./ndisc_cli
```

### Stop Service

```bash
sudo pkill ndisc_svc
```

## CLI Example Output

CLI program example output of a machine, which has 2 neighbors, each with 2 active connections:

```
=== Neighbor 1 ===
Machine ID: 9dceb1c7a7cd4121a5901618be612dfb

  Connection 1:
    Local interface: enp0s8
    Remote MAC:      08:00:27:df:e7:ba
    Remote IPv4:     none
    Remote IPv6:     fe80:0000:0000:0000:1d77:a60f:d7c2:9922

  Connection 2:
    Local interface: enp0s9
    Remote MAC:      08:00:27:da:43:fb
    Remote IPv4:     none
    Remote IPv6:     none

=== Neighbor 2 ===
Machine ID: 1c13c3d37e874add8394969b2cd7a7d9

  Connection 1:
    Local interface: enp0s8
    Remote MAC:      08:00:27:13:8c:73
    Remote IPv4:     192.168.56.10
    Remote IPv6:     fe80:0000:0000:0000:9b87:2f1f:de64:887b

  Connection 2:
    Local interface: enp0s9
    Remote MAC:      08:00:27:56:bd:e9
    Remote IPv4:     192.200.200.20
    Remote IPv6:     none

Total neighbors: 2
```


## Performance Tuning

The service is optimized for 10,000+ neighbors:

- **Socket buffer**: increased receive buffer size to 8 MB via `setsockopt()` (queues up to ~130,000 packets).
- **Memory**: pre-allocated hash maps to avoid rehashing (for neighbor and connection storage).

## Technical Details

- **No threads**: Uses `select()` for multiplexing
- **No exceptions**: Error handling via return codes
- **C++17**: Standard library only (libstdc++)
- **Platform**:  Ubuntu 24.04.3 LTS (at time of writing latest Ubuntu LTS, tested using VirtualBox)

