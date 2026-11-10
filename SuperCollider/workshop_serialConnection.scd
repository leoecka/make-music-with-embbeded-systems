SerialPort.devices;

(
// 1. Start Synth
Ndef(\saw, { | filterFreq = 1, sawFreq =  200|
    var sig;
    // sawFreq = sawFreq.lag(0.1); // smoothing data
    filterFreq = filterFreq.linexp(0,1,sawFreq,10000-sawFreq);
    sig = Saw.ar(sawFreq);
    sig = LPF.ar(sig, filterFreq);
    sig = sig!2;
}).play;

// 2. Open Serial Port
~port = SerialPort("/dev/ttyUSB0", 115200);

// 3. Routine for reading the serial data and modulating the synth
~serialRoutine = Routine({
    var ascii = Array.newClear(0);

    loop {
        var byte = ~port.read;

        // 10 = Newline (\n) -> data packet is complete
        if(byte == 10) {
            var string = ascii.collect(_.asAscii).join;
            var distance = string.asFloat;
            ascii = Array.newClear(0); // clear array for the next value

            // filtering out wrong sensor data (values over 400cm are usually errors)
            if(distance > 5 and: { distance < 400 }) {

                // Mapping: 5cm-50cm distance -> 1000Hz-100Hz
                var mappedFreq = distance.linlin(5, 50, 0.001, 1).clip(0.001, 1);

                // PRINT TO POST WINDOW:
                // .round(0.1) rounds to one decimal place for better readability
                ("Distance: " ++ distance.round(0.1) ++ " cm  ->  Frequency: " ++ mappedFreq.round(0.1) ++ " Hz").postln;

                // Update Ndef parameters
                Ndef(\saw).set(\filterFreq, mappedFreq);
            };
        } {
            // 13 = Carriage Return (\r). Ignore it, add everything else to the array.
            if(byte != 13) {
                ascii = ascii.add(byte);
            };
        };
    };
}).play;
)

// Stop everything safely and free the port
~serialRoutine.stop;
~port.close;
Ndef(\saw).stop;