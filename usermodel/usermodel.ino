#include <WiFi.h>
#include <WebServer.h>

#include <ArduinoJson.h>

const char* ssid = "ESP32_GRBL";
const char* password = "12345678";

WebServer server(80);
HardwareSerial grbl(2); // Use UART2 for GRBL (RX2=16, TX2=17)

#define BUFFER_MAX 14
int buffer_size = BUFFER_MAX;
DynamicJsonDocument doc(2048); // Global JSON document
JsonArray gcodelist;           // Global reference to JSON array
size_t gcodeIndex = 0;         // Index for tracking G-code execution

// HTML Page
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>File Upload and Spiral Generator</title>
    <style>
        /* General styles remain the same */

/* Mobile view adjustments */
@media (max-width: 600px) {
    body {
        margin: 10px;
        padding: 10px;
    }

    form, #svg-container {
        padding: 20px;
        max-width: 100%;
        margin: auto;
    }

    input, select, button {
        width: 100%;
    }

    button#movex, button#movey, button#movez, 
    button#movexn, button#moveyn, button#movezn {
        width: 100%;
        margin: 5px 0;
    }
}
p#response {
    background: #222;
    color: #0f0;
    padding: 10px;
    font-family: monospace;
    white-space: pre-wrap;
    border-radius: 5px;
    min-height: 50px;
}

/* Mobile view adjustments */
@media (max-width: 600px) {
    p#response {
        font-size: 14px; /* Slightly smaller text */
        padding: 8px;
    }
}

body {
    font-family: Arial, sans-serif;
    margin: 10px;
    padding: 10px;
    background-color: #f4f4f4;
}

h3 {
    color: #333;
}

form {
    background: #fff;
    padding: 30px;
    border-radius: 5px;
    box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    margin-bottom: 20px;
}

label {
    display: block;
    margin: 10px 0 5px;
    font-weight: bold;
}

input, select {
    min-height: 35px;
    width: 100%;
    padding: 8px;
    margin-bottom: 10px;
    border: 1px solid #ccc;
    border-radius: 4px;
}

button {
    background: #007bff;
    height: 50px;
    color: #fff;
    border: none;
    padding: 10px;
    border-radius: 5px;
    cursor: pointer;
    margin-top: 10px;
    display: block;
    width: 100%;
}

button:hover {
    background: #0056b3;
}


p#gcode {
    background: #222;
    color: #0f0;
    padding: 10px;
    font-family: monospace;
    white-space: pre-wrap;
    border-radius: 5px;
    min-height: 50px;
}

button#movex, button#movey, button#movez, button#movexn, button#moveyn, button#movezn {
    height: 50px;
    display: inline-block;
    margin: 5px 1%;
}



#svg-container {
    border: 1px solid #ddd;
    margin-top: 20px;
    padding: 10px;
    width: 90%; /* Default width for most screens */
    max-width: 500px; /* Limits maximum width */
    margin-left: auto;
    margin-right: auto;
}
/* Mobile view adjustments */
@media (max-width: 600px) {
    #svg-container {
        width: 100%; /* Full width on small screens */
        padding: 8px;
    }
}


    </style>
</head>
<body>
    <h3>Spiral Generator</h3>
    <form id='spiralForm'>
        <label for='num_turns'>Number of Turns:</label>
        <input type='number' id='num_turns' name='num_turns' value='5' step='0.1' required><br>
        
        <label for='spacing'>Spacing:</label>
        <input type='number' id='spacing' name='spacing' value='5' step='0.1' required><br>
        
        <label for='num_points'>Number of Points:</label>
        <input type='number' id='num_points' name='num_points' value='200' required><br>
        
        <label for='scale_factor'>Scale Factor:</label>
        <input type='number' id='scale_factor' name='scale_factor' value='0.2' step='0.01' required><br>

        <label for='scale_factorz'>Scale Factor Z:</label>
        <input type='number' id='scale_factorz' name='scale_factorz' value='1' step='0.01' required><br>

        <label for='degree1'>REAPEAT:</label>
        <input type='number' id='degree1' name='degree1' value='1' required><br>
        
        <label for='degree'>Rotation Degree:</label>
        <input type='number' id='degree' name='degree' value='220' required><br>
        
        <label for='feed_rate'>Feed Rate:</label>
        <input type='number' id='feed_rate' name='feed_rate' value='4999' required><br>

        <label>ADD delay :</label>
        <select id="adddelay" name="adddelay" required>
            <option value="yes">Yes</option>
            <option value="no">No</option>
        </select><br>

        <label for='delay_start'>Delay Start:</label>
        <input type='number' id='delay_start' name='delay_start' value='1' step='0.1' required><br>
        
        <label for='delay_end'>Delay End:</label>
        <input type='number' id='delay_end' name='delay_end' value='35' required><br>
        
        <label for='inner_radius'>Inner Radius:</label>
        <input type='number' id='inner_radius' name='inner_radius' value='0' required><br><br>
        
        <button type='button' id='generateBtn'>Generate Spiral</button>
    </form>
    
    <h3>SVG Visualization</h3>
    <div id='svg-container' ></div>
    
    <p id="response"></p>
    <button id='sendgcode'>RUN</button>
    
    <button id='genrateGCode'>genrateGCode</button>

    
    <input type='number' id='incvalue' name='incvalue' value='1'><br>

    <button id='movex'>X+</button>
    <button id='movey'>Y+</button>
    <button id='movez'>Z+</button>
    <button id='movexn'>X-</button>
    <button id='moveyn'>Y-</button>
    <button id='movezn'>Z-</button>
    <p id="gcode"></p>

    <br><a href='/'>[Back]</a><br><br>
    
    <script>
        let xShifted = [], yShifted = [],Xmove=0,Ymove=0,Zmove=0,INCmove,gcodeList=[];
        
        document.getElementById('generateBtn').addEventListener('click', function () {
            const numTurns = parseFloat(document.getElementById('num_turns').value);
            const spacing = parseFloat(document.getElementById('spacing').value);
            const numPoints = parseInt(document.getElementById('num_points').value);
            const scaleFactor = parseFloat(document.getElementById('scale_factor').value);

            const degree = parseFloat(document.getElementById('degree').value);
            const innerRadius = parseFloat(document.getElementById('inner_radius').value);
            
            const theta = Array.from({ length: numPoints }, (_, i) => i * (numTurns * 2 * Math.PI) / numPoints);
            const r = theta.map(t => innerRadius + spacing * t * scaleFactor);
            const x = r.map((radius, i) => radius * Math.cos(theta[i]));
            const y = r.map((radius, i) => radius * Math.sin(theta[i]));
            
            const rotationAngle = (Math.PI / 180) * degree;
            const xRotated = x.map((xi, i) => xi * Math.cos(rotationAngle) - y[i] * Math.sin(rotationAngle));
            const yRotated = y.map((yi, i) => x[i] * Math.sin(rotationAngle) + yi * Math.cos(rotationAngle));
            
            const xMin = Math.min(...xRotated), yMin = Math.min(...yRotated);
            xShifted = xRotated.map(xi => xi - xMin);
            yShifted = yRotated.map(yi => yi - yMin);
            
            const width = Math.max(...xShifted) - Math.min(...xShifted);
            const height = Math.max(...yShifted) - Math.min(...yShifted);
            
            const svgNS = 'http://www.w3.org/2000/svg';
            const svg = document.createElementNS(svgNS, 'svg');
            svg.setAttribute('width', '250');
            svg.setAttribute('height', '230');
            svg.setAttribute('viewBox', `0 0 ${width} ${height}`);
            
            const polyline = document.createElementNS(svgNS, 'polyline');
            const points = xShifted.map((xi, i) => `${xi},${yShifted[i]}`).join(' ');
            polyline.setAttribute('points', points);
            polyline.setAttribute('stroke', 'black');
            polyline.setAttribute('fill', 'none');
            polyline.setAttribute('stroke-width', '1');
            svg.appendChild(polyline);
            
            const svgContainer = document.getElementById('svg-container');
            svgContainer.innerHTML = '';
            svgContainer.appendChild(svg);
            
        });
        
document.getElementById('genrateGCode').addEventListener('click', function () {
    const feedRate = parseInt(document.getElementById('feed_rate').value);
    const delayStart = parseFloat(document.getElementById('delay_start').value);
    const delayEnd = parseFloat(document.getElementById('delay_end').value);
    const scaleFactorz = parseFloat(document.getElementById('scale_factorz').value);
    const adddelay = document.getElementById("adddelay").value.toLowerCase();
    const degree1 = parseFloat(document.getElementById('degree1').value);

    let gcode = `G21\nG90\n`;
    for (let j = 0; j < degree1; j++){
    for (let i = 0; i < xShifted.length; i++) {
        gcode += `G1 X${(xShifted[i] / 96).toFixed(3)} Y${(yShifted[i] / 96).toFixed(3)} F${feedRate}\n`;
        if (i === 0){
            gcode += `G1 Z${scaleFactorz} F${feedRate}\nM24\n`;
            if(adddelay=='yes'){
                gcode+=`G4 P${delayStart}\n`
            }
        }
    }
    gcode += `M${delayEnd}\nG0 X0 Y0 Z0\n`;
    if(degree1==1 || j==degree1-1){
        break;
    }
    gcode += `M0\n`;
   }
   gcodeList=[];
   gcodeList = gcode.trim().split("\n");
   document.getElementById("gcode").innerText = gcode;
        });

    

        document.getElementById('movex').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Xmove = parseFloat((Xmove + INCmove).toFixed(3)); // Fix floating-point error
    let gcodestr = `G0 X${Xmove}`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movexn').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Xmove = parseFloat((Xmove - INCmove).toFixed(3));
    let gcodestr = `G0 X${Xmove}`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movey').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Ymove = parseFloat((Ymove + INCmove).toFixed(3));
    let gcodestr = `G0 Y${Ymove}`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('moveyn').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Ymove = parseFloat((Ymove - INCmove).toFixed(3));
    let gcodestr = `G0 Y${Ymove}`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movez').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Zmove = parseFloat((Zmove + INCmove).toFixed(3));
    let gcodestr = `G0 Z${Zmove}`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movezn').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Zmove = parseFloat((Zmove - INCmove).toFixed(3));
    let gcodestr = `G0 Z${Zmove}`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('sendgcode').addEventListener('click', function () {
        
            fetch('/send', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ gcodelist: gcodeList })
                })
                    .then(response => response.text())
                    .then(text => document.getElementById("response").innerText = "Response: " + text)
                    .catch(error => console.error("Error:", error));

            
        });


</script>
</body>
</html>
)rawliteral";

void checkBuffer() {
    while (true) { // Keep checking until buffer condition is met
        grbl.print("?"); // Request buffer status
        //Serial.println("Sent: ?");

        String response = "";
        unsigned long startTime = millis();

        // Wait up to 200ms for a response
        while (millis() - startTime < 200) {
            while (grbl.available()) {
                char c = grbl.read();
                response += c;
            }
            if (response.indexOf("Bf:") != -1) break; // Stop waiting if "Bf:" is received
        }

        // Debugging: Show full response from GRBL
        //Serial.print("GRBL Response: ");
        //Serial.println(response);

        int pos = response.indexOf("Bf:");
        if (pos != -1) {
            int comma = response.indexOf(',', pos);
            if (comma != -1) { // Ensure comma exists
                buffer_size = response.substring(pos + 3, comma).toInt(); // First value (buffer size)
                int secondBufferValue = response.substring(comma + 1).toInt(); // Second value (should be 128)

                //Serial.print("Buffer Size: ");
                //Serial.print(buffer_size);
                //Serial.print(", Second Buffer: ");
                //Serial.println(secondBufferValue);

                if (secondBufferValue == 128) {
                    return; // Exit if second buffer value is 128
                }
            }
        }

        //Serial.println("Invalid buffer response or buffer not ready, retrying...");
        buffer_size = 0; // Reset buffer size to 0 to prevent sending G-code
        delay(100); // Prevent spamming
    }
}

String gcodeLine;
// Function to send G-code based on buffer availability
void sendNextGCode() {
    checkBuffer();
    
    while (buffer_size > 1 && gcodeIndex < gcodelist.size()) {
        // Send one G-code line to GRBL
        grbl.println(gcodelist[gcodeIndex].as<String>()); 
        //Serial.println("Sent: " + gcodelist[gcodeIndex].as<String>());
        gcodeLine = gcodelist[gcodeIndex].as<String>();

        if (gcodeLine.indexOf("G4") != -1) { // Check if the line contains "G4"
            //Serial.println("G4 detected, breaking loop.");
            checkBuffer();
        }
        gcodeIndex++;
        buffer_size--;
        
        delay(10);
        

    }

    // If all G-code is sent, reset index and clear data properly
    if (gcodeIndex >= gcodelist.size()) {
        gcodeIndex = 0;
        doc.clear();  // This is correct: Clears everything
        doc.garbageCollect();  // Helps reclaim memory
        gcodelist = doc.createNestedArray("gcodelist"); // Reinitialize empty list
    }
}

// Handle sending G-code from web
void handleSendGCode() {
    if (server.hasArg("plain")) { // Read raw JSON body
        String body = server.arg("plain");
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            //Serial.println("Invalid JSON");
            server.send(400, "text/plain", "Invalid JSON");
            return;
        }

        if (doc.containsKey("gcodelist")) {
            gcodelist = doc["gcodelist"].as<JsonArray>(); // Store global reference

            //for (String line : gcodelist) {
            //    Serial.println("Received G-code: " + line);
            //}
            server.send(200, "text/plain", "G-code chunk received successfully");
            return;
        }
    }

    server.send(200, "text/plain", "G-Code received.");
    sendNextGCode();
}


// Handle GRBL response
void handleGRBLResponse() {
    String response = "";
    while (grbl.available()) {
        char c = grbl.read();
        response += c;
    }
    server.send(200, "text/plain", response);
}

void setup() {
    //Serial.begin(115200);
    grbl.begin(115200, SERIAL_8N1, 17, 16);

    WiFi.softAP(ssid, password);
    server.on("/", []() {
        server.send_P(200, "text/html", webpage);
    });
    server.on("/send", handleSendGCode);
    server.on("/response", handleGRBLResponse);
    
    server.begin();
}

void loop() {
    server.handleClient();
    sendNextGCode();
}



