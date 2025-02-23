# smart_baby_monitor_esp32

This project demonstrates how to use the ESP32-CAM to monitor a baby’s room. The main application code is written in Arduino IDE, while the camera server application is based on esp-idf APIs.

## Features

- **Camera Sensor Setup**: Initializes the camera sensor and Wi-Fi connection.
- **Camera Server**: Starts the camera server to stream video.
- **External Sensors**: Integrates a temperature/humidity sensor and a sound detector.
  - **Temperature/Humidity Sensor**: Reads temperature and humidity every 60 seconds. Alerts are printed if values are outside specified limits.
  - **Sound Detector**: Detects sound and triggers an interrupt when sound is detected.
- **HTTP Server**: Serves a web page with the camera stream.
- **Alert System**: Monitors room conditions and prints alerts if conditions are not met.
  - **Email Notifications**: Sends alert notifications to the user via Email SMTP method.
  - **WhatsApp Notifications**: Sends alert notifications to the user via WhatsApp method.
  - **Logging**: Logs alert notifications for future reference.
- **WebSocket Support**: Enables data exchange between the ESP32 camera web server and clients.
- **Baby Status Button**: Allows the user to notify the ESP32 to switch from alarm state back to monitor state once the baby is cared for.

## Usage

1. **Setup**: Configure the camera sensor and Wi-Fi connection.
2. **Start Camera Server**: Initialize the camera server.
3. **Initialize Sensors**: Set up the temperature/humidity sensor and sound detector.
4. **Monitor Conditions**: The loop function waits for sensor interrupts and checks room conditions, printing alerts if necessary.
5. **Access Camera Stream**: Use the HTTP server to view the camera stream on a web page.

## Customization

- Easily modify the code to add more sensors or trigger different actions based on sensor readings.
- Easily modify format, style and interactivity of the web server by modifying the HTML file and using a Python script to apply the changes into the C hex array used in the source code.

## Planned Features

- **Sleep Mode**: Implement sleep mode with wakeup on certain conditions to save power.
- **Multi-Client Streaming**: Enable video streaming to multiple clients.
- **Movement Detection**: Detect movement in the baby's range to automatically disable/re-enable the monitor system.
- **Baby Sound Streaming**: Stream baby sounds.

## Getting Started

1. Clone the repository.
2. Open the project in Arduino IDE.
3. Follow the setup instructions to configure the ESP32-CAM and sensors.
4. Upload the code to your ESP32-CAM.
5. Access the camera stream via the provided HTTP server.

## Contributing

Contributions are welcome! Please fork the repository and submit a pull request.

## License

This project is licensed under the MIT License.
