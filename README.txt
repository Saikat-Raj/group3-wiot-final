Final Project P1 – BLE Contact Tracer with Exposure Logic and Energy Budget
CSCI 4270/6712 - Wireless Technologies for the Internet of Things

Group Number: [3]
Team Members: 
- Faizan Abid (ID: B01021828) - [Graduate]
- Saikat Raj (ID: B01000385) - [Graduate]
- Shiyu Huang (ID: B00985258) - [Graduate]
- Zuming Zhang (ID: B00871287) - [Graduate]


Selected Project: P1 - BLE Contact Tracer with Exposure Logic and Energy Budget
Rationale: We chose Project P1 because:
- BLE technology is highly relevant for IoT proximity detection applications
- Contact tracing represents a real-world application with significant impact
- The project combines multiple wireless IoT concepts: energy optimization, 
  RSSI-based distance estimation, persistent data storage, and wireless
  data offloading
- Allows exploration of advanced features like real-time exposure detection
  and cross-reboot state persistence using RTC memory

Work Distribution:
- Faizan Abid: 
  - BLE advertising and scanning implementation
  - Device identification and privacy management
  - Code architecture and main application logic
- Saikat Raj:
  - Energy optimization and deep sleep management
  - Data storage system (SPIFFS) and persistent state tracking
  - Battery life analysis and power profiling
- Shiyu Huang:
  - Contact tracking algorithms and exposure detection logic
  - RSSI-distance calibration experiments and data analysis
  - Statistical analysis and model fitting
- Zuming Zhang:
  - WiFi data offloading system and UDP communication
  - Real-world deployment testing and performance validation
  - Report writing and documentation
- Shared Responsibilities:
  - Hardware setup and testing
  - Data collection and validation
  - Troubleshooting and debugging

File Structure:
Group3_Final_Project/
├── README.txt                    # This file - setup and reproduction guide
├── report.pdf                    # Final technical report
├── firmware/
│   ├── group3-wiot-final.ino     # Primary contact tracing application
│   ├── constants.h               # System configuration parameters
│   └── secrets.h                 # WiFi credentials and network settings
├── data/
│   ├── compiled_data.csv         # RSSI calibration dataset (155 samples)
│   ├── deployment_logs/          # Real-world testing data
│   └── analysis_scripts/         # Python data analysis tools
├── plots/
│   ├── rssi_distance_plot.png    # RSSI vs distance calibration graph
│   ├── energy_breakdown.png      # Power consumption analysis
│   ├── performance_metrics.png   # Detection accuracy results
│   └── rssi_distance_plot.png    # RSSI vs distance calibration graph
└── server/
    └── index.js                  # Node.js UDP data collection server

Hardware Setup:
- 2x ESP32-C6 development boards (provided by course)
- 2x USB-C cables for programming and power
- 2x Laptop/PC for development and data collection
- 1x Smartphone with mobile hotspot capability (Android recommended)
- 1x Measuring tape (for RSSI-distance calibration)
- No external pin connections required
- No physical assembly required

Software Dependencies:
- Arduino IDE 2.3.6
- Data Collection Server:
  - Node.js v16.0+ 
  - Required packages: dgram, os, fs, path (built-in modules)
- Data Analysis (optional):
  - Python 3.8+
  - pandas, numpy, matplotlib, scipy (for statistical analysis)

Compilation/execution instructions:
- Connect BLE Devices with your laptops using USC-C Cable.
- Open the group3-wiot-final.ino file in Arduino IDE.
- Go to Tools and Select Board: ESP32C6 Dev Module.
- Go to Tools and Select the correct serial port for your system.
- Go to Tools and Enable: Erase All Flash Before Sketch Upload.
- Turn on the HotSpot from your device(Note: This project was tested and deployed using Android phone's HotSpot, so we highly recommended using the same system, using other hardware may or may not work).
- Enter the credentials in `secrets.h` file
- Ensure that both laptops are connected to the HotSpot as well or connected to same network that was entered in secrets.h file.
- Go to server folder and start the server by running command: `node index.js`.
- Wait for the server to start, it will provide you an IP address.
- Enter the IP Address in secrets.h file.
- Compile and Upload the code.
- Navigate to Serial Monitor Tab
- Observe the logs and check if the wifi connection is successful or not.
- Observe the logs of the UDP Server as well.


Reproduction Guide:
1. Place ESP32 devices at measured distances (1m, 2m, 4m, 7m, 10m)
2. Run the code on both laptops after completing the mentioned steps
3. Observe the logs in Serial Monitor
4. Also Observe the Console of the UDP Server
5. BLE will send data to server after 5 UDP boot counts.
6. Server will create csv files for both the devices separately in a new folder
7. Stop both the servers once you have enough data.
8. Rename the csv files or move them as these files might get overridden if program keeps running.
9. Move the devices to another distance and restart the servers again to measure readings for further scenarios.


Configuration Settings:
- SLEEP_TIME_SECONDS: 5 (device sleeps for 5 seconds between scans) // Increase it for battery optimization and reducing energy consumption
- SCAN_DURATION: 10 seconds per BLE scan
- CLOSE_CONTACT_RSSI: -60 dBm (approximately 1.5m distance)
- EXPOSURE_TIME_THRESHOLD: 300 seconds (5 minutes of close contact)
- MAX_TRACKED_DEVICES: 10 (maximum devices tracked in RTC memory)
- Upload frequency: Every 5th boot cycle (approximately every 25 seconds)

Common Issues:
1. **WiFi Connection Failed**
   - Verify credentials in `secrets.h`
   - Ensure both devices are on the same network
   - Check mobile hotspot compatibility (Android recommended)

2. **No BLE Devices Detected**
   - Verify both ESP32s are running the same firmware
   - Check if devices are in scan range
   - Monitor serial output for debugging information

3. **Server Connection Issues**
   - Ensure server is running: `node index.js`
   - Verify IP address in `secrets.h` matches server output
   - Check firewall settings on server machine
   - Check UDP port 3333 availability

4. **SPIFFS Errors**
   - Enable "Erase All Flash Before Sketch Upload"
   - Monitor SPIFFS diagnostics in serial output
   - Check available storage space

