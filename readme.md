# Do Not Sleep

Some strange storage devices (such as hard drives using hard disk cartridges) may not make it easy to set a sleep policy, so I wrote this shitty program.

## Usage

Write a config (`~/.config/do_not_sleep/conf`):

```jsonc
{
  "dirs": [
    "/mnt/hdd1",
    "/mnt/ssd1"
  ],
  "interval": 30,
  "time_range": {
    "start": [8, 0, 0],
    "end": [23, 0, 0]
  }
}
// vim: filetype=jsonc
```

Run (in a terminal, or maybe in a tmux session):

```shell
user@Machine:~$ do-not-sleep
[2023-12-15T12:00:51.340](do-not-sleep/src/ds.cc:88): "/mnt/hdd1" tick.
[2023-12-15T12:00:51.341](do-not-sleep/src/ds.cc:88): "/mnt/ssd1" tick.
[2023-12-15T12:01:21.346](do-not-sleep/src/ds.cc:88): "/mnt/hdd1" tock.
[2023-12-15T12:01:21.346](do-not-sleep/src/ds.cc:88): "/mnt/ssd1" tock.
```

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
