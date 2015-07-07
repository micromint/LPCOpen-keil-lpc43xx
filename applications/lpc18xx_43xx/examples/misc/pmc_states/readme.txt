Power Management Controller example

Example description
This example demonstrates the power states supported by PMC. The example demonstrates
steps to go to low power states & wake up from the states. 

UART needs to be setup prior to running the example as the example takes input from
the UART console.

Special connection requirements
  - Hitex LPC1850EVA-A4-2 and LPC4350EVA-A4-2 boards
    Connect a Serial cable (Straight cable) to the board's X1 (UART0) port and connect
    the other end to the host PC
    Open TeraTerm, select the COM port corresponding to the Serial Port to which the board
    is connected, select 115200 as the baud rate, 1 stop bit, no parity and no flow control
  - Keil MCB1857 and MCB4357 boards
    Connect a Serial cable (Straight cable) to the board?s P11 (UART0/3) port and connect the
    other end to the host PC
    Open TeraTerm, select the COM port corresponding to the Serial Port to which the board is
    connected, select 115200 as the baud rate, 1 stop bit, no parity and no flow control
 
Build procedures:
Visit the <a href="http://www.lpcware.com/content/project/lpcopen-platform-nxp-lpc-microcontrollers/lpcopen-v200-quickstart-guides">LPCOpen quickstart guides</a>
to get started building LPCOpen projects.