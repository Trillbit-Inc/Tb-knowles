import serial.tools.list_ports
import re
import sys
import io
import json

import sdk_com_commands
import license
import gen_json_config

print("Command line configuraion will override serial config mode.")
print("Args: <device id> <output lic/json file>")

if len(sys.argv) == 3:
    # Args: <device id> <algo config json file> <output json file>
    lic_params = {
        "customer_id": "Trillbit", # customer/company/manufacturer name. Can be public.
        "device_id": sys.argv[1] # # Unique ID of the MCU on which SDK will run. Can be public.
    }

    print(lic_params)
    
    b64_license = license.create_license(lic_params)
    b64_ck, b64_ck_nonce = license.create_ck()
    user_lic = gen_json_config.gen_config(b64_license.decode('utf-8'), 
            b64_ck.decode('utf-8'), 
            b64_ck_nonce.decode('utf-8'), 
            sys.argv[2])

    print("User Lic:", user_lic)

    sys.exit(0)

guessed_port_info = ""
count = 0
detected_ports = []
print()
print("Detected Serial Ports:")
try:
    for info in serial.tools.list_ports.comports():
        desc = info[1]
        detected_ports.append(info[0])
        if re.match(".*EDBG.*", info[1], re.IGNORECASE):
            guessed_port_info = info
            desc += " - Guessed"
        count += 1
        print("{}) {} - {}".format(count, info[0], desc))
except Exception as e:
    print("Error: ", e)
    sys.exit(-1)

if count == 0:
    print("No serial ports found.")
    sys.exit(-1)

print()
print("To select a serial port to open, enter its entry number.")
if guessed_port_info:
    print("To open guessed port ({}) just press Enter.".format(guessed_port_info[0]))
try:
    choice = input("choice: ")
    if guessed_port_info and choice == "":
        com_port = guessed_port_info[0]
    else:
        choice = int(choice)
        if (choice < 1) or (choice > len(detected_ports)):
            raise ValueError
        choice -= 1
        com_port = detected_ports[choice]
except ValueError:
    print("Invalid choice.")
    sys.exit(-1)

print()
print("Opening", com_port)

try:
    with serial.Serial(com_port, 115200, timeout=1) as ser:
        sio = io.TextIOWrapper(io.BufferedRWPair(ser, ser))
        sdk_com_commands.flush(sio)
        id = sdk_com_commands.get_serial_id(sio)
        if id:
            print("Device id:", id)
            
            lic_params = {
                "customer_id": "ABC", # customer/company/manufacturer name. Can be public.
                "device_id": id # # Unique ID of the MCU on which SDK will run. Can be public.
            }

            b64_license = license.create_license(lic_params)
            b64_ck, b64_ck_nonce = license.create_ck()
            
            lic_bytes = gen_json_config.gen_config(b64_license.decode('utf-8'), 
                                b64_ck.decode('utf-8'), 
                                b64_ck_nonce.decode('utf-8'), 
                                None)

            #print("License ({}): {}".format(len(b64_license), b64_license))
            print("Sending License")
            ok = sdk_com_commands.set_license(sio, lic_bytes)
            if ok:
                print("Done.")
            else:
                print("Failed to set license")
except KeyboardInterrupt:
    print("Aborting.")
except Exception as e:
    print("Error:", e)

print(com_port, "closed.")
