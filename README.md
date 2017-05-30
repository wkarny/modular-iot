# modular-iot
Modular IoT setup is beneficial for implementing flexible and scalable sensor network system .
Hybrid Sensor Network is chosen to be the preferred network architecture for the present
setup. As the majority of sensor nodes are battery powered, they are connected in Ad Hoc
Mode. The Mains powered nodes operate in Infrastructure Mode. This setup facilitates
dynamic addition and deletion of sensor blocks as well as register/deregister for different
services. Various protocols for Hybrid Sensor Network (HSN) are used as part of the
proposed system. A small scale working prototype is implemented for demonstration
purpose.

Dependencies:
RF24 library: https://github.com/nRF24/RF24.git

How to use:
1. Import project
2. You have to install lamp/xamp to your system.
3. Then copy web-iot to the htdocs folder of lamp/xamp.
4. Start lamp/xamp
5. Make files
	  $ make
6. Start Gateway 
	  $ sudo ./gateway


