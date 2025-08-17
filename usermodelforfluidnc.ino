#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h> // Include LittleFS library

const char* ssid = "ESP32_GRBL";
const char* password = "12345678";

WebServer server(80);
HardwareSerial grbl(2); // Use UART2 for GRBL (RX2=16, TX2=17)

#define BUFFER_MAX 14
int buffer_size = BUFFER_MAX;
DynamicJsonDocument doc(2048); // Global JSON document
JsonArray gcodelist;          // Global reference to JSON array
size_t gcodeIndex = 0;        // Index for tracking G-code execution

// Define preset filenames
const char* NEW_PRESET_FILE = "/new_preset.json";
const char* MINI_PRESET_FILE = "/mini_preset.json";
const char* THREE_ROUND_PRESET_FILE = "/three_round_preset.json";
const char* FOUR_ROUND_PRESET_FILE = "/four_round_preset.json";
const char* BAGANI_PRESET_FILE = "/bagani_preset.json";

// Default preset values (used for initial file creation)
const char* defaultNewPreset = R"({"num_points":200,"degree1":1,"degree":220,"feed_rate":1050,"adddelay":"yes","delay_start":1.0,"delay_end":0.1,"completeafter":"no","adddelayend":"no","delay_stop":1.0,"goinside":"no","goinsidex":1.0,"goinsidey":1.0,"press":"no","pressz":1.0,"addslowstart":"yes","num_turns":5.0,"scale_factorz":1.0,"scale_factor":500.0,"inner_radius":0.0,"startfeed":0.0,"tillpoint":0.0})";
const char* defaultMiniPreset = R"({"num_points":200,"degree1":1,"degree":220,"feed_rate":1050,"adddelay":"yes","delay_start":1.0,"delay_end":0.1,"completeafter":"no","adddelayend":"no","delay_stop":1.0,"goinside":"no","goinsidex":1.0,"goinsidey":1.0,"press":"no","pressz":1.0,"addslowstart":"yes","num_turns":5.0,"scale_factorz":1.0,"scale_factor":500.0,"inner_radius":0.0,"startfeed":0.0,"tillpoint":0.0})";
const char* defaultThreeRoundPreset = R"({"num_points":200,"degree1":1,"degree":220,"feed_rate":1050,"adddelay":"yes","delay_start":1.0,"delay_end":0.1,"completeafter":"no","adddelayend":"no","delay_stop":1.0,"goinside":"no","goinsidex":1.0,"goinsidey":1.0,"press":"no","pressz":1.0,"addslowstart":"yes","num_turns":5.0,"scale_factorz":1.0,"scale_factor":500.0,"inner_radius":0.0,"startfeed":0.0,"tillpoint":0.0})";
const char* defaultFourRoundPreset = R"({"num_points":200,"degree1":1,"degree":220,"feed_rate":1050,"adddelay":"yes","delay_start":1.0,"delay_end":0.1,"completeafter":"no","adddelayend":"no","delay_stop":1.0,"goinside":"no","goinsidex":1.0,"goinsidey":1.0,"press":"no","pressz":1.0,"addslowstart":"yes","num_turns":5.0,"scale_factorz":1.0,"scale_factor":500.0,"inner_radius":0.0,"startfeed":0.0,"tillpoint":0.0})";
const char* defaultBaganiPreset = R"({"num_points":200,"degree1":1,"degree":220,"feed_rate":1050,"adddelay":"yes","delay_start":1.0,"delay_end":0.1,"completeafter":"no","adddelayend":"no","delay_stop":1.0,"goinside":"no","goinsidex":1.0,"goinsidey":1.0,"press":"no","pressz":1.0,"addslowstart":"yes","num_turns":5.0,"scale_factorz":1.0,"scale_factor":500.0,"inner_radius":0.0,"startfeed":0.0,"tillpoint":0.0})";

// HTML Page (remains the same as provided, but JavaScript will be modified)
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

.preset-group {
    display: flex;
    gap: 10px;
    margin-bottom: 10px;
    flex-wrap: wrap; /* Allows wrapping on smaller screens */
}

.preset-group button {
    flex: 1; /* Distributes space evenly */
    min-width: 80px; /* Minimum width for buttons */
}

.preset-group .save-button {
    background-color: #28a745; /* Green for save buttons */
}

.preset-group .save-button:hover {
    background-color: #218838;
}

#moreOptionsToggle {
    background-color: #6c757d;
}

#moreOptionsToggle:hover {
    background-color: #5a6268;
}

#moreOptions {
    border: 1px solid #ddd;
    padding: 15px;
    margin-bottom: 20px;
    border-radius: 5px;
    background-color: #f9f9f9;
}


    </style>
</head>
<body>
    <h3>Spiral Generator</h3>

    <div class="preset-group">
        <button type="button" id="newBtn">New</button>
        <button type="button" class="save-button" id="saveNewBtn">Save New</button>
    </div>
    <div class="preset-group">
        <button type="button" id="miniBtn">Mini</button>
        <button type="button" class="save-button" id="saveMiniBtn">Save Mini</button>
    </div>
    <div class="preset-group">
        <button type="button" id="threeRoundBtn">3 Round</button>
        <button type="button" class="save-button" id="saveThreeRoundBtn">Save 3 Round</button>
    </div>
    <div class="preset-group">
        <button type="button" id="fourRoundBtn">4 Round</button>
        <button type="button" class="save-button" id="saveFourRoundBtn">Save 4 Round</button>
    </div>
    <div class="preset-group">
        <button type="button" id="baganiBtn">Bagani</button>
        <button type="button" class="save-button" id="saveBaganiBtn">Save Bagani</button>
    </div>

    <form id='spiralForm'>
        <label for='feed_rate'>Feed Rate:</label>
        <input type='number' id='feed_rate' name='feed_rate' value='1050' required><br>

        <label for='scale_factorz'>Scale Factor Z:</label>
        <input type='number' id='scale_factorz' name='scale_factorz' value='1' step='0.01' required><br>

        <label for='scale_factor'>Scale Factor:</label>
        <input type='number' id='scale_factor' name='scale_factor' value='500' step='0.01' required><br>

        <label for='num_turns'>Number of Turns:</label>
        <input type='number' id='num_turns' name='num_turns' value='5' step='0.1' required><br>

        <label for='inner_radius'>Inner Radius:</label>
        <input type='number' id='inner_radius' name='inner_radius' value='0' required><br><br>

        <label for='startfeed'>Start Feed Rate:</label>
        <input type='number' id='startfeed' name='startfeed' value='0' required><br><br>

        <label for='tillpoint'>Till Point (Ramp End Index):</label>
        <input type='number' id='tillpoint' name='tillpoint' value='0' required><br><br>

        <button type="button" id="moreOptionsToggle">More Options</button>
        <div id="moreOptions" style="display: none;">
            <label for='num_points'>Number of Points:</label>
            <input type='number' id='num_points' name='num_points' value='200' required><br>
            
            <label for='degree1'>REPEAT:</label>
            <input type='number' id='degree1' name='degree1' value='1' required><br>
            
            <label for='degree'>Rotation Degree:</label>
            <input type='number' id='degree' name='degree' value='220' required><br>

            <label>ADD delay:</label>
            <select id="adddelay" name="adddelay" required>
                <option value="yes">Yes</option>
                <option value="no">No</option>
            </select><br>

            <label for='delay_start'>Delay Start:</label>
            <input type='number' id='delay_start' name='delay_start' value='1' step='0.1' required><br>
            
            <label for='delay_end'>Delay End:</label>
            <input type='number' id='delay_end' name='delay_end' value='0.1' required><br>
            
            <label>Complete After:</label>
            <select id="completeafter" name="completeafter" required>
                <option value="no">No</option>
                <option value="yes">Yes</option>
            </select><br>

            <label>ADD delay before end:</label>
            <select id="adddelayend" name="adddelayend" required>
                <option value="no">No</option>
                <option value="yes">Yes</option>
            </select><br>
            <label for='delay_stop'>Delay Stop:</label>
            <input type='number' id='delay_stop' name='delay_stop' value='1' step='0.1' required><br>

            <label>Go inside:</label>
            <select id="goinside" name="goinside" required>
                <option value="no">No</option>
                <option value="yes">Yes</option>
            </select><br>
            <label for='goinsidex'>X (Go Inside):</label>
            <input type='number' id='goinsidex' name='goinsidexp' value='1' step='0.1' required><br>
            <label for='goinsidey'>Y (Go Inside):</label>
            <input type='number' id='goinsidey' name='goinsidey' value='1' step='0.1' required><br>

            <label>Press:</label>
            <select id="press" name="press" required>
                <option value="no">No</option>
                <option value="yes">Yes</option>
            </select><br>
            <label for='pressz'>Z (Press):</label>
            <input type='number' id='pressz' name='pressz' value='1' step='0.1' required><br>
            <label>Add Slow Start:</label>
            <select id="addslowstart" name="addslowstart" required>
                <option value="yes">Yes</option>
                <option value="no">No</option>
            </select><br>
        </div>
        
        <button type='button' id='generateBtn'>Generate Spiral</button>
    </form>
    
    <h3>SVG Visualization</h3>
    <div id='svg-container' ></div>

    <label for='command'>WRITE COMMAND</label>
    <input type='text' id='command' name='command'><br>
    <button id='addtolist'>ADD TO LIST</button>

    <p id="response"></p>
    <button id='sendgcode'>RUN</button>
    
    <button id='genrateGCode'>Generate G-Code</button>

    
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
        let xShifted = [], yShifted = [], Xmove = 0, Ymove = 0, Zmove = 0, INCmove, gcodeList = [];

        // Function to load preset values into the form from ESP32
        async function loadPreset(presetName) {
            try {
                const response = await fetch(`/loadPreset?name=${presetName}`);
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                const presetValues = await response.json();
                for (const key in presetValues) {
                    const element = document.getElementById(key);
                    if (element) {
                        if (element.type === 'number') {
                            element.value = presetValues[key];
                        } else if (element.tagName === 'SELECT') {
                            element.value = presetValues[key];
                        }
                    }
                }
            } catch (error) {
                console.error("Error loading preset:", error);
                alert(`Error loading preset "${presetName}". Check console for details.`);
            }
        }

        // Function to save current form values to a preset on ESP32
        async function savePreset(presetName) {
            const currentValues = {};
            // Get all form elements and their values
            const form = document.getElementById('spiralForm');
            const elements = form.elements;
            for (let i = 0; i < elements.length; i++) {
                const element = elements[i];
                if (element.id && element.name) { // Ensure element has an ID and name
                    if (element.type === 'number') {
                        currentValues[element.id] = parseFloat(element.value);
                    } else if (element.tagName === 'SELECT') {
                        currentValues[element.id] = element.value;
                    }
                }
            }
            
            try {
                const response = await fetch(`/savePreset?name=${presetName}`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(currentValues)
                });
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                const text = await response.text();
                alert(`Preset "${presetName}" saved! Response: ${text}`);
            } catch (error) {
                console.error("Error saving preset:", error);
                alert(`Error saving preset "${presetName}". Check console for details.`);
            }
        }

        // Event Listeners for Preset Buttons
        document.getElementById('newBtn').addEventListener('click', () => loadPreset('new'));
        document.getElementById('miniBtn').addEventListener('click', () => loadPreset('mini'));
        document.getElementById('threeRoundBtn').addEventListener('click', () => loadPreset('threeRound'));
        document.getElementById('fourRoundBtn').addEventListener('click', () => loadPreset('fourRound'));
        document.getElementById('baganiBtn').addEventListener('click', () => loadPreset('bagani'));

        // Event Listeners for Save Buttons
        document.getElementById('saveNewBtn').addEventListener('click', () => savePreset('new'));
        document.getElementById('saveMiniBtn').addEventListener('click', () => savePreset('mini'));
        document.getElementById('saveThreeRoundBtn').addEventListener('click', () => savePreset('threeRound'));
        document.getElementById('saveFourRoundBtn').addEventListener('click', () => savePreset('fourRound'));
        document.getElementById('saveBaganiBtn').addEventListener('click', () => savePreset('bagani'));

        // Toggle visibility of "More Options"
        document.getElementById('moreOptionsToggle').addEventListener('click', function() {
            const moreOptionsDiv = document.getElementById('moreOptions');
            if (moreOptionsDiv.style.display === 'none') {
                moreOptionsDiv.style.display = 'block';
                this.textContent = 'Hide More Options';
            } else {
                moreOptionsDiv.style.display = 'none';
                this.textContent = 'More Options';
            }
        });

        // Initialize with default/new preset values on page load
        document.addEventListener('DOMContentLoaded', () => {
            loadPreset('new'); // Load the 'new' preset on page load
        });
        
        document.getElementById('generateBtn').addEventListener('click', function () {
            const numTurns = parseFloat(document.getElementById('num_turns').value);
            const numPoints = parseInt(document.getElementById('num_points').value);
            const scaleFactor = parseFloat(document.getElementById('scale_factor').value);
            const degree = parseFloat(document.getElementById('degree').value);
            const innerRadius = parseFloat(document.getElementById('inner_radius').value);
            
            const thetaMax = numTurns * 2 * Math.PI;
            const theta = Array.from({ length: numPoints }, (_, i) => i * thetaMax / (numPoints - 1));

            // Spiral grows from innerRadius to fixed scaleFactor
            const r = theta.map(t => innerRadius + (scaleFactor - innerRadius) * (t / thetaMax));

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
            document.getElementById('genrateGCode').click();
        });
        
document.getElementById('genrateGCode').addEventListener('click', function () {
    const feedRate = parseInt(document.getElementById('feed_rate').value);
    const delayStart = parseFloat(document.getElementById('delay_start').value);
    const delayEnd = parseFloat(document.getElementById('delay_end').value);
    const scaleFactorz = parseFloat(document.getElementById('scale_factorz').value);
    const adddelay = document.getElementById("adddelay").value.toLowerCase();
    const degree1 = parseFloat(document.getElementById('degree1').value);
    const adddelayend = document.getElementById("adddelayend").value.toLowerCase();
    const delaystop = parseFloat(document.getElementById('delay_stop').value);
    const goinside = document.getElementById("goinside").value.toLowerCase();
    const press = document.getElementById("press").value.toLowerCase();
    const goinsidex = parseFloat(document.getElementById('goinsidex').value);
    const goinsidey = parseFloat(document.getElementById('goinsidey').value);
    const pressz = parseFloat(document.getElementById('pressz').value);
    const completeafter = document.getElementById("completeafter").value.toLowerCase();

    // NEW VARIABLES FOR SLOW START
    const addslowstart = document.getElementById("addslowstart").value.toLowerCase(); // static assignment for now
    const startFeedRate = parseInt(document.getElementById('startfeed').value); // starting feed rate
    const rampUntilIndex = parseInt(document.getElementById('tillpoint').value); // how many points to ramp up until

    let gcode = `G21\nG90\n`;

    for (let j = 0; j < degree1; j++) {
        for (let i = 0; i < xShifted.length; i++) {
            // Compute ramped feed rate if enabled and within range
            let currentFeed = feedRate;
            if (addslowstart === "yes" && i < rampUntilIndex) {
                let rampRatio = i / rampUntilIndex;
                currentFeed = Math.round(startFeedRate + rampRatio * (feedRate - startFeedRate));
            }
            if (i === 0) {
                gcode += `G1 X${(xShifted[i] / 96).toFixed(3)} Y${(yShifted[i] / 96).toFixed(3)} F${feedRate}\n`;
                gcode += `G1 Z${scaleFactorz} F${feedRate}\nM64 P0\n`;
                if (adddelay === 'yes') {
                    gcode += `G4 P${delayStart}\n`;
                }
            }else{
                gcode += `G1 X${(xShifted[i] / 96).toFixed(3)} Y${(yShifted[i] / 96).toFixed(3)} F${currentFeed}\n`;
            }
        }

        if (completeafter === 'yes') {
            gcode += `M65 P0\nG4 P0\n`;
        } else {
            gcode += `G4 P0\nM65 P0\n`;
        }

        gcode += `M64 P1\nG4 P${delayEnd}\nM65 P1\n`;

        if (goinside === 'yes') {
            const finalX = ((xShifted[xShifted.length - 1] / 96) + goinsidex).toFixed(3);
            const finalY = ((yShifted[yShifted.length - 1] / 96) + goinsidey).toFixed(3);
            gcode += `G1 X${finalX} Y${finalY} F${feedRate}\n`;
        }

        if (press === 'yes') {
            gcode += `G1 Z${scaleFactorz + pressz} F${feedRate}\n`;
        }

        if (adddelayend === 'yes') {
            gcode += `G4 P${delaystop}\n`;
        }

        gcode += `G1 X0 Y0 Z0 F${feedRate}\n`;

        if (degree1 == 1 || j == degree1 - 1) {
            break;
        }

        gcode += `M0\n`;
    }

    gcodeList = [];
    gcodeList = gcode.trim().split("\n");
    document.getElementById("gcode").innerText = gcode;
});


        document.getElementById('movex').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Xmove = parseFloat((Xmove + INCmove).toFixed(3)); // Fix floating-point error
    let gcodestr = `G1 X${Xmove} F500`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('addtolist').addEventListener('click', function () {
    const commandInput = document.getElementById('command').value;
    let gcodestr = commandInput;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movexn').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Xmove = parseFloat((Xmove - INCmove).toFixed(3));
    let gcodestr = `G1 X${Xmove} F500`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movey').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Ymove = parseFloat((Ymove + INCmove).toFixed(3));
    let gcodestr = `G1 Y${Ymove} F500`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('moveyn').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Ymove = parseFloat((Ymove - INCmove).toFixed(3));
    let gcodestr = `G1 Y${Ymove} F500`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movez').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Zmove = parseFloat((Zmove + INCmove).toFixed(3));
    let gcodestr = `G1 Z${Zmove} F500`;
    gcodeList=[];
    gcodeList.push(gcodestr);
    document.getElementById("gcode").innerText = gcodestr;
});

document.getElementById('movezn').addEventListener('click', function () {
    let INCmove = parseFloat(document.getElementById('incvalue').value);
    Zmove = parseFloat((Zmove - INCmove).toFixed(3));
    let gcodestr = `G1 Z${Zmove} F500`;
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

// --- LittleFS Helper Functions ---

// Reads content of a file from LittleFS
String readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    //Serial.println(F("Failed to open file for reading"));
    return "";
  }
  String fileContent = file.readString();
  file.close();
  return fileContent;
}

// Writes content to a file in LittleFS
void writeFile(const char* path, const char* content) {
  File file = LittleFS.open(path, "w");
  if (!file) {
    //Serial.println(F("Failed to open file for writing"));
    return;
  }
  if (file.print(content)) {
    //Serial.println(F("File written successfully"));
  } else {
    //Serial.println(F("File write failed"));
  }
  file.close();
}

// --- Web Server Handlers for Presets ---

void handleLoadPreset() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Missing preset name");
    return;
  }
  String presetName = server.arg("name");
  const char* filename;

  if (presetName == "new") {
    filename = NEW_PRESET_FILE;
  } else if (presetName == "mini") {
    filename = MINI_PRESET_FILE;
  } else if (presetName == "threeRound") {
    filename = THREE_ROUND_PRESET_FILE;
  } else if (presetName == "fourRound") {
    filename = FOUR_ROUND_PRESET_FILE;
  } else if (presetName == "bagani") {
    filename = BAGANI_PRESET_FILE;
  } else {
    server.send(404, "text/plain", "Preset not found");
    return;
  }

  String presetContent = readFile(filename);
  if (presetContent.length() > 0) {
    server.send(200, "application/json", presetContent);
  } else {
    server.send(404, "text/plain", "Preset file empty or not found on ESP32.");
  }
}

void handleSavePreset() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Missing preset name");
    return;
  }
  String presetName = server.arg("name");
  const char* filename;

  if (presetName == "new") {
    filename = NEW_PRESET_FILE;
  } else if (presetName == "mini") {
    filename = MINI_PRESET_FILE;
  } else if (presetName == "threeRound") {
    filename = THREE_ROUND_PRESET_FILE;
  } else if (presetName == "fourRound") {
    filename = FOUR_ROUND_PRESET_FILE;
  } else if (presetName == "bagani") {
    filename = BAGANI_PRESET_FILE;
  } else {
    server.send(404, "text/plain", "Preset not found");
    return;
  }

  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    writeFile(filename, body.c_str());
    server.send(200, "text/plain", "Preset saved successfully.");
  } else {
    server.send(400, "text/plain", "No data to save.");
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
        doc.clear();   // This is correct: Clears everything
        doc.garbageCollect();   // Helps reclaim memory
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


void setup() {
    Serial.begin(115200);
    grbl.begin(115200, SERIAL_8N1, 17, 16);
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    Serial.println("LittleFS mounted successfully");

    // Check if preset files exist, if not, create them with default values
    if (!LittleFS.exists(NEW_PRESET_FILE)) {
        writeFile(NEW_PRESET_FILE, defaultNewPreset);
    }
    if (!LittleFS.exists(MINI_PRESET_FILE)) {
        writeFile(MINI_PRESET_FILE, defaultMiniPreset);
    }
    if (!LittleFS.exists(THREE_ROUND_PRESET_FILE)) {
        writeFile(THREE_ROUND_PRESET_FILE, defaultThreeRoundPreset);
    }
    if (!LittleFS.exists(FOUR_ROUND_PRESET_FILE)) {
        writeFile(FOUR_ROUND_PRESET_FILE, defaultFourRoundPreset);
    }
    if (!LittleFS.exists(BAGANI_PRESET_FILE)) {
        writeFile(BAGANI_PRESET_FILE, defaultBaganiPreset);
    }
    Serial.println("file create successfully");


    WiFi.softAP(ssid, password);
    server.on("/", []() {
        server.send_P(200, "text/html", webpage);
    });
    server.on("/send", handleSendGCode);
    server.on("/response", handleGRBLResponse);
    server.on("/loadPreset", handleLoadPreset); // New endpoint for loading presets
    server.on("/savePreset", HTTP_POST, handleSavePreset); // New endpoint for saving presets
    
    server.begin();
}

void loop() {
    server.handleClient();
    sendNextGCode();
}