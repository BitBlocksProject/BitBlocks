Tor Support
===========

BitBlocks Core can connect through Tor and can also advertise a Tor hidden service. This is optional; normal clearnet operation uses mainnet P2P port `58697`.

Connect through Tor
-------------------

Install and start Tor, then add these options to `bitblocks.conf` if Tor listens on the local default SOCKS port:

```ini
proxy=127.0.0.1:9050
listen=1
```

Restart BitBlocks Core and inspect peers:

```bash
bitblocks-cli getnetworkinfo
bitblocks-cli getpeerinfo
```

Hidden service
--------------

To accept inbound Tor connections, configure Tor with a hidden service that forwards to the BitBlocks P2P port.

Example `torrc`:

```text
HiddenServiceDir /var/lib/tor/bitblocks-service/
HiddenServicePort 58697 127.0.0.1:58697
```

Reload Tor, then read the generated hostname:

```bash
sudo systemctl reload tor
sudo cat /var/lib/tor/bitblocks-service/hostname
```

Add the address to `bitblocks.conf`:

```ini
listen=1
externalip=yourgeneratedaddress.onion
```

Restart `bitblocksd` after changing the configuration.

Operational notes
-----------------

- Do not reuse old onion node lists from other networks.
- Keep your firewall open only for ports you intend to serve.
- Tor can be slower than clearnet; initial synchronization may take longer.
- Masternodes should only use Tor if the network and operator policy support that topology.
