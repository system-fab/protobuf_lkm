# Netfilter

## Instructions

1. CD into example directory
```shell
cd <example_folder>
```
2. Compile module
```shell
make -C module
```
3. Install module
```shell
make -C module install
```
4. Load module
```shell
make -C module load
```
5. Compile and run server app
```shell
make -c user_space server 
```
6. Compile and run client app (in a new shell)
```shell
make -c user_space client
```

> You should see logs using `dmesg`

## Examples

### Hello World
This hook drops all messages where Foo.bar > 50

### Address Book
Drops messages that have less than 2 `WORK` phone number inside the the first `Person.Phones`.

### Floats
Drops messages where Foo.bar > 10.1 .