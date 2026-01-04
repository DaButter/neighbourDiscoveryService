## Stuff done un VMs:

Connected VMs via ```Settings -> Network -> Adapter 2```, set to Internal Network, intnet, Allow All (Pomiscous mode for test purposes, so all traffic is seen on the eth interface).


To share clipboard + dragNdrop
```Devices -> Insert Guest Additions CD image```

After that run:
```sh
sudo apt update
sudo apt install build-essential dkms linux-headers-$(uname -r)
cd /media/$USER/VBox_GAs_*
sudo ./VBoxLinuxAdditions.run
```
Reboot vm


For other stuff:
```sh
sudo apt install openssh-server
```
To access VMs, need to enable port forwarding in Adatper 1: host port 2222, guest port 22, name SSH
connect via: ssh -p 2222 username@127.0.0.1
for other VMs, forward a different host port, f.e. 2223


## Add ips to interfaces

```bash
sudo ip addr add 192.168.56.10/24 dev enp0s8
sudo ip addr add 2001:db8::10/64 dev enp0s8
ip addr show enp0s8
```

del
```bash
sudo ip addr del 192.168.56.10/24 dev enp0s8
sudo ip addr del 2001:db8::10/64 dev enp0s8
```

Test down interfaces and mac addr updates
```bash
sudo ip link set enp0s8 down
sudo ip link set enp0s8 address 02:01:02:03:04:05
sudo ip link set enp0s8 up
```