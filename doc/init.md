Service Configuration for bitblocksd
====================================

This document provides sample service configuration for running `bitblocksd` as a background service. The repository does not currently ship ready-made files under `contrib/init`, so copy and adapt the examples below for your host.

Service user
------------

Create a dedicated user and directories:

```bash
sudo useradd --system --home /var/lib/bitblocksd --shell /usr/sbin/nologin bitblocks
sudo mkdir -p /etc/bitblocks /var/lib/bitblocksd
sudo chown bitblocks:bitblocks /var/lib/bitblocksd
sudo chmod 700 /var/lib/bitblocksd
```

Minimal configuration
---------------------

Create `/etc/bitblocks/bitblocks.conf`:

```ini
rpcuser=bitblocks_rpc
rpcpassword=change_this_to_a_strong_random_password
rpcport=59768
port=58697
server=1
daemon=1
datadir=/var/lib/bitblocksd
```

Protect the file because it contains RPC credentials:

```bash
sudo chown bitblocks:bitblocks /etc/bitblocks/bitblocks.conf
sudo chmod 600 /etc/bitblocks/bitblocks.conf
```

systemd example
---------------

Create `/etc/systemd/system/bitblocksd.service`:

```ini
[Unit]
Description=BitBlocks daemon
After=network-online.target
Wants=network-online.target

[Service]
User=bitblocks
Group=bitblocks
Type=forking
ExecStart=/usr/local/bin/bitblocksd -conf=/etc/bitblocks/bitblocks.conf -datadir=/var/lib/bitblocksd -daemon
ExecStop=/usr/local/bin/bitblocks-cli -conf=/etc/bitblocks/bitblocks.conf -datadir=/var/lib/bitblocksd stop
Restart=on-failure
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true
TimeoutStopSec=120

[Install]
WantedBy=multi-user.target
```

Enable and start it:

```bash
sudo systemctl daemon-reload
sudo systemctl enable bitblocksd
sudo systemctl start bitblocksd
sudo systemctl status bitblocksd
```

OpenRC example
--------------

Create `/etc/init.d/bitblocksd`:

```sh
#!/sbin/openrc-run
name="bitblocksd"
description="BitBlocks daemon"
command="/usr/local/bin/bitblocksd"
command_args="-conf=/etc/bitblocks/bitblocks.conf -datadir=/var/lib/bitblocksd -daemon"
command_user="bitblocks:bitblocks"
pidfile="/var/lib/bitblocksd/bitblocksd.pid"
start_stop_daemon_args="--wait 1000"

depend() {
    need net
}
```

Then run:

```bash
sudo chmod +x /etc/init.d/bitblocksd
sudo rc-update add bitblocksd default
sudo rc-service bitblocksd start
```

Upstart example
---------------

Create `/etc/init/bitblocksd.conf` on systems that still use Upstart:

```text
description "BitBlocks daemon"
start on runlevel [2345]
stop on runlevel [016]
respawn
setuid bitblocks
setgid bitblocks
exec /usr/local/bin/bitblocksd -conf=/etc/bitblocks/bitblocks.conf -datadir=/var/lib/bitblocksd -daemon=0
```

Troubleshooting
---------------

- Check `debug.log` under `/var/lib/bitblocksd`.
- Confirm that `/usr/local/bin/bitblocksd` and `/usr/local/bin/bitblocks-cli` match your install path.
- Confirm that the RPC password in `bitblocks.conf` is the same file used by both the daemon and CLI.
