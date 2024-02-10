# Do Not Sleep

Some strange storage devices (such as hard drives using hard disk cartridges) may not make it easy to set a sleep policy, so I wrote this shitty program.

## Usage

Write a config (`~/.config/do_not_sleep/conf`), run (in a terminal, or maybe in a tmux session, or [systemd unit](./example/systemd/do-not-sleep.service)):

```shell
user@Machine:~$ do-not-sleep
[2023-12-15T12:00:51.340](do-not-sleep/src/ds.cc:88): "/mnt/hdd1" tick.
[2023-12-15T12:00:51.341](do-not-sleep/src/ds.cc:88): "/mnt/ssd1" tick.
[2023-12-15T12:01:21.346](do-not-sleep/src/ds.cc:88): "/mnt/hdd1" tock.
[2023-12-15T12:01:21.346](do-not-sleep/src/ds.cc:88): "/mnt/ssd1" tock.
```

### Time range mode

```jsonc
{
  "dirs": [
    "/mnt/disk1",
    "/mnt/disk2"
  ],
  // every 2 minutes
  "interval": 120,
  "policy": "time_range",
  "time_range": {
    "start": [8, 0, 0],
    "end": [23, 0, 0]
  }
}
// vim: filetype=jsonc
```

this program will check if it is now within `time_range`, if so, disks in `dirs` are kept awake.

### Monitor IO mode (Unavailable)

> CURRENTLY UNAVAILABLE

```jsonc
{
  "dirs": [
    "/mnt/disk1",
    "/mnt/disk2"
  ],
  // every 2 minutes
  "interval": 120,
  "policy": "monitor_io",
  "monitor_io": {
    // scan I/O operations every 1 second
    "scan_frequency": 1,
    // keep awake for 30 minutes
    "keep_awake": 1800
  },
}
```

this program will monitor I/O operations of disks in `dirs`, if a I/O is monitored, disks in `dirs` are kept awake for the duration of `keep_awake`.

### Service available mode

```jsonc
{
  "dirs": [
    "/mnt/disk1",
    "/mnt/disk2"
  ],
  // every 2 minutes
  "interval": 120,
  "policy": "service_available",
  // tcp 10.0.0.1:22
  "service_available": "10.0.0.1:22"
}
```

this program will check if a TCP connection can be established with `service_available`, if so, disks in `dirs` are kept awake.

## Essence

Write random data to those dirs periodly.

## Build

Requires [jsoncpp](https://github.com/open-source-parsers/jsoncpp) for parsing config.

```sh
cmake -B build
cmake --build build

# output
./build/do-not-sleep
```
