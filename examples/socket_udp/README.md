# UDP Socket

This folder contains examples of LKM that open UDP sockets in the kernel and decode received protobuf messages.

## Instructions
1. CD into the example directory
```shell
cd <example_folder>
```
2. Compile module
```shell
make -C module build
```
3. Install module
```shell
make -C module install
```
4. Load module
```shell
make -C module load
```
5. Compile and run userspace app
```shell
make -C user_space run
```
6. Unload the module
```shell
make -C module unload
```
> For some reason if you do not do this some logs will not appear

7. You should see logs like these using `dmesg`
```shell
root@ubuntu:~# dmesg
[  326.742819] example_mod_udp:example_mod_udp_init(): Loaded module
[  332.166743] example_mod_udp:example_mod_udp_init(): Received data, decoding...
[  332.166749] example_mod_udp:process_message(): people.length: 1
[  332.166749] example_mod_udp:process_message(): person_p->id: 56
[  332.166750] example_mod_udp:process_message(): phone_number_p->number_p: +46701232345
[  356.315550] example_mod_udp:example_mod_udp_exit(): removed
```
> The printed data varies between examples

