> :warning: The Cartesi team keeps working internally on the next version of this repository, following its regular development roadmap. Whenever there's a new version ready or important fix, these are published to the public source tree as new releases.

# Cartesi Machine Emulator Tools Example

### Requirements

- Docker >= 18.x
- GNU Make >= 3.81
- genext2fs >= 1.5.0
- bsdtar >= 3.6.2

### Building

The following example demostrates how to create a bootable cartesi machine based on a Ubuntu RISC-V 64 docker image. It requires the machine-emulator-tools v0.10.0.

```sh
cp ../machine-emulator-tools-v0.10.0.tar.gz .
./build ubuntu-22.04.dockerfile
```

After the build is done, the file `ubuntu-22.04.ext2` should be available in the current path.
Assuming you have `cartesi-machine` installed in your environment,
you can then test interactive terminal with it:

```sh
$ cartesi-machine --flash-drive=label:root,filename:ubuntu-22.04.ext2 -i bash
Running in interactive mode!

         .
        / \
      /    \
\---/---\  /----\
 \       X       \
  \----/  \---/---\
       \    / CARTESI
        \ /   MACHINE
         '

root@cartesi-machine:~#
```
