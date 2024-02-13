# Arduino Device Manager

## Overview

This project comprises an Arduino sketch (`Arduino.ino`) and a Python script (`Arduino.py`) designed to manage a network of devices through serial communication. It enables the addition, update, and removal of devices, along with the adjustment of their settings such as output power and operational state. The system is ideal for environments where remote device management is required, offering a versatile platform for various applications.

## Features

- **Device Management**: Add, update, and remove devices dynamically.
- **Adjustable Settings**: Modify device settings, including output power and state.
- **Serial Communication**: Interface with the Arduino using a Python script over a serial connection.

## Getting Started

### Prerequisites

- Arduino IDE
- Python 3.x
- PySerial package for Python
- An Arduino board (e.g., Uno, Mega)

### Setup

1. **Arduino Setup**:
   - Upload the `Arduino.ino` sketch to your Arduino board using the Arduino IDE.

2. **Python Environment**:
   - Ensure Python 3.x is installed on your system.
   - Install PySerial using pip:
     ```
     pip install pyserial
     ```

3. **Connecting Arduino to Your Computer**:
   - Connect the Arduino board to your computer via USB.
   - Note the port name (e.g., COM3 on Windows, /dev/ttyACM0 on Linux/Mac).

### Running the Project

1. **Start the Python Script**:
   - Run the Python script with the port name as an argument:
     ```
     python Arduino.py COM3
     ```
   - Replace `COM3` with the appropriate port name for your system.

2. **Interacting with the System**:
   - The Python script sends predefined commands to the Arduino to manage devices.
   - Modify the `msgs` list in `Arduino.py` to customize the commands sent to the Arduino.

## Customization

- **Arduino Sketch**:
  - Modify device management logic within `Arduino.ino` as per your requirements.
- **Python Script**:
  - Adjust the `msgs` list in `Arduino.py` to change the commands and device settings.

## Troubleshooting

- Ensure correct drivers are installed for your Arduino board.
- Verify the serial port name and baud rate match between the Arduino sketch and Python script.
- Check the Arduino and computer are correctly connected via USB.

## Contributing

We welcome contributions! If you have suggestions or want to improve the project, please feel free to fork the repository and submit a pull request.

## License

This project is open source and available under the [MIT License](LICENSE).

---

