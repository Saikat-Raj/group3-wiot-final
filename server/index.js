const dgram = require('dgram');
const os = require('os');
const fs = require('fs');
const path = require('path');

// Get local IP address (IPv4)
function getLocalIP() {
  const interfaces = os.networkInterfaces();
  for (const name of Object.keys(interfaces)) {
    for (const iface of interfaces[name]) {
      if (iface.family === 'IPv4' && !iface.internal) {
        return iface.address;
      }
    }
  }
  return '127.0.0.1'; // fallback
}

const LOCAL_IP = getLocalIP();
const PORT = 3333;

// Create data directory if it doesn't exist
const DATA_DIR = path.join(__dirname, 'received_data');
if (!fs.existsSync(DATA_DIR)) {
  fs.mkdirSync(DATA_DIR);
}

const server = dgram.createSocket('udp4');

server.on('error', (err) => {
  console.log(`Server error:\n${err.stack}`);
  server.close();
});

server.on('message', (msg, rinfo) => {
  console.log(`\n=== Data received from ESP32 device ${rinfo.address}:${rinfo.port} ===`);
  
  const data = msg.toString().trim();
  console.log('Raw data:');
  console.log(data);
  
  // Handle empty data case
  if (!data || data.length === 0) {
    console.log('⚠️  WARNING: Received empty data packet!');
    console.log('This could mean:');
    console.log('  1. ESP32 is not in upload cycle (bootCount % 5 != 0)');
    console.log('  2. No Bluetooth scan data collected yet');
    console.log('  3. SPIFFS file system is empty');
    console.log('===============================================\n');
    
    // Still send ACK for empty packets
    server.send('ACK', rinfo.port, rinfo.address);
    return;
  }
  
  // Parse CSV data - now supports both old and new formats
  const lines = data.split('\n');
  console.log('\nParsed BLE contact data:');
  
  let validEntries = 0;
  let uploadTimestamp = null;
  
  lines.forEach((line, index) => {
    const trimmedLine = line.trim();
    
    // Skip empty lines
    if (!trimmedLine) return;
    
    // Handle upload timestamp comment
    if (trimmedLine.startsWith('# Upload Timestamp:')) {
      uploadTimestamp = trimmedLine.split(':')[1].trim();
      console.log(`Upload Timestamp: ${uploadTimestamp} (${new Date(parseInt(uploadTimestamp) * 1000).toISOString()})`);
      return;
    }
    
    // Skip CSV headers
    if (trimmedLine.includes('timeStamp,peerId,rssi')) {
      console.log(`CSV Header detected: ${trimmedLine}`);
      return;
    }
    
    // Parse data lines
    const parts = trimmedLine.split(',');
    if (parts.length >= 3) {
      const timestamp = parts[0];
      const peerId = parts[1];
      const rssi = parts[2];
      const deviceId = parts[3] || 'N/A';
      const uploadDuration = parts[4] || 'N/A';
      
      // Validate timestamp
      const timestampNum = parseInt(timestamp);
      if (isNaN(timestampNum) || timestampNum <= 0) {
        console.log(`Entry ${index}: Invalid timestamp: ${timestamp}`);
        return;
      }
      
      const date = new Date(timestampNum * 1000);
      if (parts.length >= 5) {
        // New format with deviceId and uploadDuration
        console.log(`Entry ${index}: Time: ${date.toISOString()}, Peer ID: ${peerId}, RSSI: ${rssi} dBm, Device: ${deviceId}, Duration: ${uploadDuration}ms`);
      } else {
        // Old format
        console.log(`Entry ${index}: Time: ${date.toISOString()}, Peer ID: ${peerId}, RSSI: ${rssi} dBm`);
      }
      validEntries++;
    }
  });
  
  console.log(`\nTotal valid entries: ${validEntries}`);
  if (uploadTimestamp) {
    console.log(`Upload completed at: ${new Date(parseInt(uploadTimestamp) * 1000).toISOString()}`);
  }
  console.log('===============================================\n');
  
  // Only save to file if we have actual data
  if (validEntries > 0) {
    // Save data to file (creates one file per device)
    const deviceFileName = `device_${rinfo.address.replace(/\./g, '_')}_data.csv`;
    const filePath = path.join(DATA_DIR, deviceFileName);
    
    // Check if file exists and create header if it's a new file
    const fileExists = fs.existsSync(filePath);
    
    if (!fileExists) {
      console.log(`Creating new data file: ${deviceFileName}`);
      // Create file with updated CSV header to match new format
      const csvLines = data.split('\n');
      const headerLine = csvLines.find(line => line.includes('timeStamp,peerId,rssi'));
      const header = headerLine || 'timeStamp,peerId,rssi,deviceId,uploadDuration';
      fs.writeFileSync(filePath, header + '\n');
    }
    
    // Append data to device-specific file
    fs.appendFile(filePath, data + '\n', (err) => {
      if (err) {
        console.error(`Error saving data from ${rinfo.address}:`, err.message);
      } else {
        console.log(`Data saved to: ${filePath}`);
      }
    });
  } else {
    console.log('No valid data entries found - skipping file save');
  }
  
  // Send ACK response as expected by ESP32
  server.send('ACK', rinfo.port, rinfo.address);
});

server.on('listening', () => {
  const address = server.address();
  console.log(`UDP server listening on ${address.address}:${address.port}`);
});

server.bind(PORT, LOCAL_IP);