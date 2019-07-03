Super simple status bar for DWM (for linux).

Gets autoconfigured during make.

The default compiler is clang. The program can be built with any sane C compiler (gcc, tcc, etc). Eg.:

```bash
make CC=gcc
```
**PLEASE NOTE: The makefile will try to infer the wifi interface name, but may require manual change in case the net.ifnames is enabled (predictable interface names)**
