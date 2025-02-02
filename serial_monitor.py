import serial
import serial.tools.list_ports
import time

# List of popular baud rates to try
baud_rates = [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]

def find_active_port():
    print("Scanning for available serial ports... Please wait.")
    time.sleep(2)  # Adds a 2-second delay for scanning to make it feel more interactive
    
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found.")
        return None

    print("Available serial ports:")
    for port in ports:
        print(f"- {port.device} ({port.description})")

    for port in ports:
        print(f"\nTrying to connect to {port.device}...")
        for baudrate in baud_rates:
            print(f"Trying baud rate: {baudrate}...")
            try:
                ser = serial.Serial(port.device, baudrate, timeout=1)
                ser.reset_input_buffer()
                # Try reading a line to detect activity
                data = ser.readline()
                if data or ser.in_waiting:
                    print(f"Successfully connected to {port.device} at {baudrate} baud.")
                    return ser
                ser.close()
            except PermissionError:
                print(f"Failed to connect at {baudrate} baud. Error: Permission denied. Make sure the port is not being used by another program and try running as Administrator.")
                continue
            except (serial.SerialException, OSError) as e:
                print(f"Failed to connect at {baudrate} baud. Error: {e}")
                continue

    print("No active serial devices detected.")
    return None

def read_serial_data():
    print("Attempting to detect an active serial port...")
    ser = find_active_port()

    if ser:
        try:
            print("\nSerial data connection established. Listening for data...")
            while True:
                if ser.in_waiting:
                    data = ser.readline().decode('utf-8', errors='ignore').strip()
                    print(f"Received data: {data}")
                else:
                    print("No data received, still waiting...")
                    exit(1)
        except KeyboardInterrupt:
            print("\nStopped by user.")
        finally:
            print("Closing serial connection...")
            ser.close()
    else:
        print("No active serial devices found. Exiting...")

def main():
    print("Welcome to the Serial Port Auto-Detect Tool!")
    input("Press Enter to start scanning for serial ports...\n")
    read_serial_data()

if __name__ == "__main__":
    main()
