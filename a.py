import serial
import struct

def read_device_id(port, baudrate=115200):
    with serial.Serial(port, baudrate, timeout=1) as ser:
        # Example command to read the DBG_IDCODE register (0x4001 5800)
        # This assumes a bootloader protocol allowing register reads
        read_cmd = struct.pack('<BI', 0x11, 0x40015800)  # Custom read command
        ser.write(read_cmd)
        response = ser.read(4)  # Assuming a 4-byte response
        if len(response) == 4:
            device_id = struct.unpack('<I', response)[0]
            print(f"Device ID: 0x{device_id:08X}")
        else:
            print("Failed to read Device ID")

def read_uid(port, baudrate=115200):
    with serial.Serial(port, baudrate, timeout=1) as ser:
        # Read 16 bytes from UID register (0x1FFF0E00)
        read_cmd = struct.pack('<BI', 0x11, 0x1FFF0E00)
        ser.write(read_cmd)
        response = ser.read(16)
        if len(response) == 16:
            uid = response.hex().upper()
            print(f"UID: {uid}")
        else:
            print("Failed to read UID")

if __name__ == "__main__":
    serial_port = "/dev/cu.usbserial-11230"  # Change this to your serial port (e.g., /dev/ttyUSB0 on Linux)
    read_device_id(serial_port)
    read_uid(serial_port)
