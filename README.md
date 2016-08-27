# NetMO

Tiny tcp library and a few tests on this library, including echoserver.
Now works under Linux only. 
C++11 compiler required.
To test servers and clients you may see on 'make_ip_link.sh' shell script.
It creates virtual ip-link between two virtual net interfaces.
Test virtual link by 'ping' or other tools for example:

~ ping -I veth0 'ip-address-on-veth1' 

Learn this script, learn net bridges and try to write script that creates virtual links of type one-to-many
