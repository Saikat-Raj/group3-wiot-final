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
  
  // Parse CSV data (timeStamp,peerId,rssi format)
  const lines = data.split('\n');
  console.log('\nParsed BLE contact data:');
  
  lines.forEach((line, index) => {
    const trimmedLine = line.trim();
    if (trimmedLine && trimmedLine !== 'timeStamp,peerId,rssi') {
      const [timestamp, peerId, rssi] = trimmedLine.split(',');
      if (timestamp && peerId && rssi) {
        const date = new Date(parseInt(timestamp) * 1000);
        console.log(`Entry ${index}: Time: ${date.toISOString()}, Peer ID: ${peerId}, RSSI: ${rssi} dBm`);
      }
    }
  });
  
  console.log('===============================================\n');
  
  // Save data to file (optional - creates one file per device)
  const deviceFileName = `device_${rinfo.address.replace(/\./g, '_')}_data.csv`;
  const filePath = path.join(DATA_DIR, deviceFileName);
  
  // Append data to device-specific file
  fs.appendFile(filePath, data + '\n', (err) => {
    if (err) {
      console.error(`Error saving data from ${rinfo.address}:`, err.message);
    } else {
      console.log(`Data saved to: ${filePath}`);
    }
  });
  
  // Send ACK response as expected by ESP32
  server.send('ACK', rinfo.port, rinfo.address);
});

server.on('listening', () => {
  const address = server.address();
  console.log(`UDP server listening on ${address.address}:${address.port}`);
});

server.bind(PORT, LOCAL_IP);