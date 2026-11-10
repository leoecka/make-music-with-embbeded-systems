SerialPort.devices;

(
// 1. Synth starten
Ndef(\saw, { | filterFreq = 1, sawFreq =  200|
	var sig;
	//sawFreq = sawFreq.lag(0.1);
	filterFreq = filterFreq.linexp(0,1,sawFreq,10000-sawFreq);
	sig = Saw.ar(sawFreq);
	sig = LPF.ar(sig, filterFreq);
	sig = sig!2;
}).play;

// 2. Serial Port öffnen
~port = SerialPort("/dev/ttyUSB0", 115200);

// 3. Routine zum Auslesen und Mappen der Werte
~serialRoutine = Routine({
	var ascii = Array.newClear(0);

	loop {
		var byte = ~port.read;

		// 10 = Newline (\n) -> Datenpaket ist komplett
		if(byte == 10) {
			var string = ascii.collect(_.asAscii).join;
			var distance = string.asFloat;
			ascii = Array.newClear(0); // Array zurücksetzen für den nächsten Wert

			// Sensor-Fehlmessungen filtern
			if(distance > 5 and: { distance < 400 }) {

				// Mapping: 5cm-50cm Distanz -> 1000Hz-100Hz
				var mappedFreq = distance.linlin(5, 50, 0.001, 1).clip(0.001, 1);

				// HIER WIRD GEPRINTET:
				// .round(0.1) rundet auf eine Nachkommastelle für bessere Lesbarkeit
				("Distanz: " ++ distance.round(0.1) ++ " cm  ->  Frequenz: " ++ mappedFreq.round(0.1) ++ " Hz").postln;

				// Ndef updaten
				Ndef(\saw).set(\filterFreq, mappedFreq);
			};
		} {
			// 13 = Carriage Return (\r). Wird ignoriert. Alles andere ans Array anhängen.
			if(byte != 13) {
				ascii = ascii.add(byte);
			};
		};
	};
}).play;
)

//stop
~serialRoutine.stop;
~port.close;
Ndef(\saw).stop;