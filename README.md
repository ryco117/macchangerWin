macchangerWin
=============

Is a fully featured MAC address spoofer for windows! It is an easy to use command line tool that requires 
almost no prior knowledge about the newtork cards you wish to spoof the MAC address of. It will generate a random 
MAC address by default, but you can specify an address to spoof of your own, or reset back to the default using the command line 
argument "-r". It needs to be run with admin privileges in order to work because it needs to edit the registry (for spoofing the address) 
and to take down the network interface temporarily (so that a restart isn't needed for the spoof to take effect).

Example Args
------------
```
macchangerWin			        ;This will assign a random MAC address to the specified NIC
macchangerWin -h		        ;This displays arg help menu
macchangerWin -r		        ;This resets the MAC back to the default (stops spoofing)
macchangerWin -m A1B2C3D4E5F6	;This spoofs the MAC address to A1B2C3D4E5F6. Note the address needs to be formatted in this way with caps and no colons (despite using colons being more common)
```

