# ESP32 G-Code Transmitter  

## Overview  
This project enables an ESP32 to act as a G-code transmitter, receiving G-code from a web app and sending it to an Arduino Uno running GRBL via TX/RX (pins 17 and 16). The web interface can be customized by the developer, and G-code commands can be dynamically added to a G-code list and sent to the ESP32 via POST requests.  
## API Endpoints  

### `POST /send`  
**Description:** Adds a G-code command to the list and sends it to the ESP32.  

## Features  
- Web-based interface for sending G-code  
- Customizable webpage for different applications  
- Sends G-code commands to GRBL on Arduino Uno  
- Uses TX (GPIO17) and RX (GPIO16) for serial communication  

## Hardware Requirements  
- ESP32  
- Arduino Uno with GRBL firmware  
- CNC Machine (or stepper motor setup)  
- Power supply and necessary wiring  

## Software Requirements  
- Arduino IDE with ESP32 board support  
- GRBL firmware for Arduino Uno  
- Web app for sending G-code (HTML, JavaScript, or a framework like React)  

## Installation  

1. **Flash ESP32 Firmware**  
   - Install the required ESP32 libraries in the Arduino IDE  
   - Upload the provided ESP32 firmware  
   
2. **Set Up Web App**  
   - Modify the webpage as needed  
   - Host it on ESP32 or an external server  

3. **Connect Hardware**  
   - Connect ESP32 TX (GPIO17) to Arduino RX  
   - Connect ESP32 RX (GPIO16) to Arduino TX  
   - Ensure common ground between ESP32 and Arduino  

