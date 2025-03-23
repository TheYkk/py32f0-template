import serial
import time

def send_command(ser, command):
    ser.write(bytes([command, command ^ 0xFF]))
    ack = ser.read(1)
    return ack == b'\x79'  # Check for ACK response

def get_chip_info(port, baudrate=115200):
    with serial.Serial(port, baudrate, timeout=1) as ser:
        time.sleep(0.1)
        
        # Send initial 0x7F to enter bootloader
        ser.write(b'\x7F')
        ack = ser.read(1)
        if ack != b'\x79':
            print("No response from bootloader. Ensure the STM32 is in boot mode.")
            return
        
        # Get bootloader version
        if send_command(ser, 0x01):
            length = ord(ser.read(1))  # Number of bytes to read
            version = ser.read(1)
            ser.read(length)  # Read remaining bytes
            ser.read(1)  # Read ACK
            print(f"Bootloader Version: 0x{version.hex().upper()}")
        else:
            print("Failed to get bootloader version.")
            return
        
        # Get chip ID
        if send_command(ser, 0x02):
            length = ord(ser.read(1))  # Should be 1 (length)
            chip_id = ser.read(2)  # 2 bytes for chip ID
            ser.read(1)  # Read ACK
            print(f"Chip ID: 0x{chip_id.hex().upper()}")
        else:
            print("Failed to get chip ID.")

if __name__ == "__main__":
    port = "/dev/cu.usbserial-11230"
    get_chip_info(port)