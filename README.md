# trivial-rootkit
Basic rootkit developed as a learning exercise

No guarantee is given that this won't just break your kernel- it probably will

Developed for: Ubuntu 32-bit 

Usage:
After running ```make```, load the kernel module with
```sudo insmod not_a_rootkit.ko```
After enabling write permissions on the device file ```/dev/not_a_rootkit```, the followings commands can be given via 
```echo "{command}" > /dev/not_a_rootkit```

Privilege escalation
Raise current process to root
```do-root0```
Raise a process with given PID to root
```do-root{pid}```

Enable Stealth
The rootkit can be hidden from basic programs like ```ls``` and ```lsmod``` with
```do-hide```
Note this will also hide all files with the prefix ```not_a_rootkit```, including the device file. However, this file is still there and can still be written to.

Hide process
A process with a given PID can be hidden with 
```do-hideproc{pid}```
The current rootkit implementation also allows hiding of arbitrarily named files with the same command
```do-hideproc{filename}```


Removing the rootkit cleanly after its been hidden with ```do-hide``` is a work in progress- remove it by rebooting the system instead.
