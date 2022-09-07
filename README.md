# Setup Boards
Connect the IA611 Xplained Pro extension board to **EXT2** connector of SAMD21 Xplained Pro board as shown below.

![Board connections](IA611-SAMD21-DEMO1/Documents/images/board.jpg)

# Setup Serial Terminal Program
You can use any serial terminal program to view the output from Demo application.
This document will use Tera Term.

Demo application uses EDBG UART to communicate with PC/User.

Connect the *DEBUG USB* port of the board to PC with a micro USB cable and open Tera Term. EDBG COM Port number on your PC may be different.

![COM Port Selection](IA611-SAMD21-DEMO1/Documents/images/tera_term_com.jpg)

Demo application configures EDBG UART as 115200 8-n-1.

To set this settings in Tera Term, select Menu *Setup -> Serial Port*
and enter values as shown below, then click button *New Settings*.

![Serial Port Setup](IA611-SAMD21-DEMO1/Documents/images/tera_term_serial_setting.jpg)

To correctly display Demo output in Tera Term, select Menu *Setup -> Terminal*
and enter values as shown below, then click button *OK*. Ignore the locale, output will be only ASCII.

![Serial Port Setup](IA611-SAMD21-DEMO1/Documents/images/tera_term_terminal_setting.jpg)

# Flash the Demo Binary
For convenience, the demo binary is located at Binfiles/IA611-SAMD21-DEMO1.bin

Connect the *DEBUG USB* port of the board to PC with a micro USB cable. On Windows OS, File explorer will open up showing the *XPLAINED* drive contents.

Copy-Paste the demo binary to this *XPLAINED* drive. Copying process may take some time as on-chip debugger flashes the binary. NOTE: You will not see the binary file in the *XPLAINED* drive once copying is done. Check serial terminal output after copying is done.

# Getting Trillbit Host SDK License
If this is the first time board was flashed or reflashed after chip erase then you will 
see a license not found message on serial terminal.

![Unlicensed](IA611-SAMD21-DEMO1/Documents/images/unlicensed.jpg)

## Setup Python Environment
Ensure you have python3 installed on your PC.
Open command prompt shell (Not Power Shell) and run the following commands in the *scripts* folder.

```
> python -m venv py_env
> py_env\Scripts\activate.bat
> pip install pyserial
```

Before running the license program disconnect Tera Term from Menu *File -> Disconnect*,
otherwise license program will report permission or busy error.

Once python setup is complete run the license program in the *scripts* folder.

Follow the instructions the from the license program. If successful you will see LED0 on SAMD21 board blink once when IA61x is configured. And blink 4 times when Trillbit algorithm accepts authentication and is ready for data detection.

# Using Demo
After License was downloaded in the previous step, SAMD21 was ready to detect data however since the Tera Term was disconnected you won't see any messages.

Connect Tera Term again and press the *RESET* switch on SAMD21 board. 
You will messages as below on the terminal. When you see the message indicating algorithm is ready for data detection, play some data sound. Point the phone/speaker towards IA611 board. Currently only SSI0 is supported.

The number before *Data Detected* event indicates how many events were detected till now. The number in the Payload parentheses indicates payload length. 

![Success Output](IA611-SAMD21-DEMO1/Documents/images/success.jpg)

# Provisioning Commands over UART
The general format of commands sent from PC to board is:
```
tbc <command> [args] [checksum][\r]\n
```

Where:

- Each field is separated by a single space character.

- *tbc* is the Trillbit command prefix to distinguish ascii serial data.

- *command* is integer command code. Decimal format.

- *args* command arguments if present.

- *checksum* Some arguments may be protected by 8-bit check sum. Checksum is computed by summing the data sequence and returning 8-bit LSB bits. Decimal format.

- *\r* - Optional Carriage Return (CR) character after command.

- *\n* - Mandatory New Line/Line Feed character after command.
  
The general format for response from board to PC is:
```
tbr <error_code> [data]\n
```

Where:

- Each field is separated by a single space character.

- *tbr* is Trillbit response prefix to distinguish ascii serial data.

- *error_code* 0 on success, else negative error code integer. Decimal format.

- *data* Data fields depending on command.

- *\n* New line termination of response.

**NOTE:** *\r*, *\n* characters are displayed here only for representation. On actual serial terminal they won't be visible.

## Command - Get MCU/Device ID
Command:
```
tbc 2\n
```
Response:
```
tbr 0 <id_string> <checksum>\n
```

Response Example:
```
tbr 0 ABCDEF12345 148\n
```

*checksum* is applied only to *id_string*. Note *id_string* will not contain spaces.

## Command - Set License
Command:
```
tbc 6 <base64_encoded_license_string> <checksum>\n
```

*checksum* is applied only to license string. Note *base64_encoded_license_string* will not contain spaces.

Response:
```
tbr 0\n
```

Success response code will be received only when *checksum* given in command matches with that computed by the board firmware, else negative error code will be returned.

Refer to *provision* module source code for error code details.
