LWIP FreeRTOS TCP Echo example

Example description
Welcome to the LWIP TCP Echo example using the NET API for RTOS based
operation. This example shows how to use the NET API with the LWIP contrib
TCP Echo (threaded) example using the 18xx/43xx LWIP MAC and PHY drivers.
The example shows how to handle PHY link monitoring and indicate to LWIP that
a ethernet cable is plugged in.

To use the example, Simply connect an ethernet cable to the board. The board
will acquire an IP address via DHCP and you can ping the board at it's IP
address. You can monitor network traffice to the board using a tool such as
wireshark at the boards MAC address.

Special connection requirements
There are no special connection requirements for this example.

Build procedures:
Visit the <a href="http://www.lpcware.com/content/project/lpcopen-platform-nxp-lpc-microcontrollers/lpcopen-v200-quickstart-guides">LPCOpen quickstart guides</a>
to get started building LPCOpen projects.