# Example: Cartesi root filesystem with machine emulator tools

The following example demonstrates how to create a bootable root filesystem for the Cartesi Machine based on a Ubuntu RISC-V 64 docker image. It requires the machine-emulator-tools v0.13.0.

### Requirements

- Docker >= 18.x
- GNU Make >= 3.81
- genext2fs >= 1.5.0
- bsdtar >= 3.6.2
- bc >= 1.06

### Building

```sh
cp ../machine-emulator-tools-v0.13.0.tar.gz .
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
