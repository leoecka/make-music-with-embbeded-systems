(
// ================================================================
// STEP 1: Find your USB Port
// ================================================================
// Run this single line first! Look at the Post Window to find your port.
SerialPort.devices;
)


(
// ================================================================
// STEP 2: Run the Synthesizer & Test via GUI
// ================================================================

// 1. Define the Synthesizer
Ndef(\saw, { | filterFreq = 0.5, sawFreq = 200 |
    var sig, cutoff;

    // .lag ensures smooth transitions between incoming serial values
    sawFreq = sawFreq.lag(0.05);
    filterFreq = filterFreq.lag(0.05);

    // Internal mapping: 0.001-1.0 from Serial -> 200Hz-9000Hz Cutoff
    cutoff = filterFreq.linexp(0.001, 1.0, 200, 9000);

    sig = Saw.ar(sawFreq);
    sig = LPF.ar(sig, cutoff);
    sig = sig ! 2; // Duplicate to Stereo
}).play;

// 2. Add Specifications for the GUI sliders
// Format: [min_value, max_value, mapping_curve, step_size, default_value]
Ndef(\saw).addSpec(\filterFreq, [0.001, 1.0, \lin, 0.001, 0.5]);
Ndef(\saw).addSpec(\sawFreq, [100, 1000, \exp, 1, 200]);

// 3. Open the Graphical User Interface (GUI)
// Try moving the sliders with your mouse before connecting the sensors!
Ndef(\saw).gui;
)

(
// ================================================================
// STEP 3: Connect Sensors via USB Serial
// ================================================================

// 1. Open Serial Port
// ---> WORKSHOP STUDENTS: PASTE YOUR PORT NAME HERE <---
// Windows example: "COM3"
// Mac example: "/dev/cu.usbserial-0001"
~port = SerialPort("/dev/ttyUSB0", 115200);

// 2. Serial Reading & Parsing Routine
~serialRoutine = Routine({
    var ascii = Array.newClear(0);

    loop {
        var byte = ~port.read;

        // 10 = Newline (\n) -> End of data packet
        if(byte == 10) {
            var string = ascii.collect(_.asAscii).join;
            var values = string.split($,); // Split string at comma

            // Proceed only if we received exactly 2 values
            if(values.size == 2) {
                var distance = values[0].asFloat;
                var ldrValue = values[1].asFloat;

                // Match with ESP32 Core 1 Mapping:
                // Map Distance (5cm-50cm) to Sawtooth Pitch (100Hz to 1000Hz)
                var mappedSaw = distance.linlin(5, 50, 100, 1000).clip(100, 1000);

                // Map LDR (200-3800) to Filter Base Value (0.001 to 1.0)
                var mappedFilter = ldrValue.linlin(200, 3800, 0.001, 1.0).clip(0.001, 1.0);

                // Update Synth parameters
                Ndef(\saw).set(\filterFreq, mappedFilter, \sawFreq, mappedSaw);

                // Print values to Post Window for debugging
                ("Distance: " ++ distance.round(0.1) ++ " cm | LDR: " ++ ldrValue.asInteger).postln;
            };

            // Clear array for the next incoming packet
            ascii = Array.newClear(0);
        } {
            // 13 = Carriage Return (\r). Ignore it, add everything else to array.
            if(byte != 13) {
                ascii = ascii.add(byte);
            };
        };
    };
}).play;
)

// ================================================================
// STEP 4: Stop Everything Safely
// ================================================================
// Run these lines to stop the sound and free the USB port:
// ~serialRoutine.stop;
// ~port.close;
// Ndef(\saw).stop;