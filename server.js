/*
Submission For the Facebook Online Hackathon 2020
Created by Zul Yang
Server Code 
*/

//Particle Photon Settings
var fs = require("fs");
var Particle = require('particle-api-js');
var particle = new Particle();
var token;
var settings = {
	ip: "192.168.1.141",
	port: 3443
};

particle.login({username: 'zulyang94@gmail.com', password: 'XXXX'}).then(
  function(data) {
    token = data.body.access_token;
  },
  function (err) {
    console.log('Could not log in.', err);
  }
);

//Twillio Settings
const accountSid = 'AC3668fb57298fc1a779a414d913e7d9de';
const authToken = '703dc67590b284883a4ef310d271eb29';
const twilio_client = require('twilio')(accountSid, authToken);

//Microphone Conversion Settings
var samplesLength = 1000;
var sampleRate = 16000;
var bitsPerSample = 8;
var numChannels = 1;
var outStream = fs.createWriteStream("generated.wav");

//Creation of Header for .Wav File
var writeHeader = function() {
	var b = new Buffer(1024);
	b.write('RIFF', 0);
	/* file length */
	b.writeUInt32LE(32 + samplesLength * numChannels, 4);
	//b.writeUint32LE(0, 4);

	b.write('WAVE', 8);

	/* format chunk identifier */
	b.write('fmt ', 12);

	/* format chunk length */
	b.writeUInt32LE(16, 16);

	/* sample format (raw) */
	b.writeUInt16LE(1, 20);

	/* channel count */
	b.writeUInt16LE(1, 22);

	/* sample rate */
	b.writeUInt32LE(sampleRate, 24);

	/* byte rate (sample rate * block align) */
	b.writeUInt32LE(sampleRate * 1, 28);
	//b.writeUInt32LE(sampleRate * 2, 28);

	/* block align (channel count * bytes per sample) */
	b.writeUInt16LE(numChannels * 1, 32);
	//b.writeUInt16LE(2, 32);

	/* bits per sample */
	b.writeUInt16LE(bitsPerSample, 34);

	/* data chunk identifier */
	b.write('data', 36);

	/* data chunk length */
	//b.writeUInt32LE(40, samplesLength * 2);
	b.writeUInt32LE(0, 40);

	outStream.write(b.slice(0, 50));
};

writeHeader(outStream);

//Initializing connection to Photon
var net = require('net');
console.log("connecting...");
client = net.connect(settings.port, settings.ip, function () {
	client.setNoDelay(true);
    client.on("data", function (data) {
        try {
			outStream.write(data);
			//outStream.flush();
			console.log("got chunk of " + data.length + " bytes ");
			console.log("got chunk of " + data.toString('HEX'));
		}
        catch (ex) {
            console.error("Er!" + ex);
		}
		finally{
		}
	});	
})
//Sending the .wav file to wit.ai
var WitSpeech = require("node-witai-speech");
//var fs = require("fs");
// Stream the file to be sent to the wit.ai
var stream = fs.createReadStream("/Users/zulyang/RescueMe/generated.wav"); //Path to your .wav file here. 
console.log('READ');
// The wit.ai instance api key
var API_KEY = "XXXXX";

// The content-type for this audio stream (audio/wav, ...)
var content_type = "audio/wav";

var parseSpeech =  new Promise((ressolve, reject) => {
	// call the wit.ai api with the created stream
	WitSpeech.extractSpeechIntent(API_KEY, stream, content_type, 
	(err, res) => {
		if (err) return reject(err);
		ressolve(res);
	});
	console.log('READ2');
});

// check in the promise for the completion of call to witai
parseSpeech.then((data) => {
	console.log(data);
	//If data.intent is the help, then activate below.
	//If it is a crisis, then send to Particle API and Twillio API.
	var fnPr = particle.callFunction({ deviceId: 'XXXX', name: 'runServo', argument: 'Open', auth: token });
	fnPr.then(
		function(data) {
			console.log('Called Servo Function Succesfully:', data);
		}, function(err) {
			console.log('An error occurred:', err);
		});

	//Send WhatApp Message
	//Twillio API Settings
	twilio_client.messages
		.create({
			from: 'whatsapp:+14155238886',
			body: 'Your Neighbour May Be In Danger!',
			to: 'whatsapp:+6592212557'
			})
		.then(message => console.log(message.sid));
		console.log('Sent Whatsapp Message to +6592212557')
})
.catch((err) => {
	console.log(err);
})

;

setTimeout(function() {
	console.log('recorded for 10 seconds');
	client.end();
	outStream.end();
	process.exit(0);
}, 10 * 1000);