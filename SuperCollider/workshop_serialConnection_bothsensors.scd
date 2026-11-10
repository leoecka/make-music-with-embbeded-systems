(
// ================================================================
// STEP 1: Find your USB Port
// ================================================================
// Run this single line first! Look at the Post Window to find your port.
SerialPort.devices;
)


(
// ================================================================
// STEP 2: Run the Synthesizer
// ================================================================

// 1. Define the Synthesizer
Ndef(\saw, { | filterFreq = 0.5, sawFreq = 200 |
    var sig;

    // .lag ensures smooth transitions between incoming serial values
    sawFreq = sawFreq.lag(0.05);
    filterFreq = filterFreq.lag(0.05);

    // Internal mapping: 0.0-1.0 from Serial -> 200Hz-9000Hz Cutoff
    var cutOff = filterFreq.linexp(0.001, 1.0, 200, 9000);

    sig = Saw.ar(sawFreq);
    sig = LPF.ar(sig, cutOff);
    sig = sig ! 2; // Duplicate to Stereo
}).play;

// 2. Open Serial Port
// ---> WORKSHOP STUDENTS: PASTE YOUR PORT NAME HERE <---
// Windows example: "COM3"
// Mac example: "/dev/cu.usbserial-0001"
~port = SerialPort("YOUR_PORT_NAME_HERE", 115200);

// 3. Serial Reading & Parsing Routine
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

                // Map Distance (5cm-50cm) to Filter Base Value (0.001 to 1.0)
                var mappedFilter = distance.linlin(5, 50, 0.001, 1.0).clip(0.001, 1.0);

                // Map LDR (200-3800) to Sawtooth Pitch (100Hz to 1000Hz)
                var mappedSaw = ldrValue.linlin(200, 3800, 100, 1000).clip(100, 1000);

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
// STEP 3: Stop Everything Safely
// ================================================================
// Run these lines to stop the sound and free the USB port:
 ~serialRoutine.stop;
 ~port.close;
 Ndef(\saw).stop;