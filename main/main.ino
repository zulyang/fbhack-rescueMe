/*
Submission For the Facebook Online Hackathon 2020
Created by Zul Yang
MicroController Code (Particle)
*/

#define MICROPHONE_PIN A5
#define AUDIO_BUFFER_MAX 8192

//Microphone Settings
int audioStartIdx = 0, audioEndIdx = 0;
uint16_t audioBuffer[AUDIO_BUFFER_MAX];
uint16_t txBuffer[AUDIO_BUFFER_MAX];
unsigned long lastRead = micros();
char myIpAddress[24];

//Servo Settings
Servo myservo;  
int pos = 0;   

//PIR Sensor Settings
int PIRPin = D1;  
int counter = 0;

//Particle Settings
TCPClient audioClient;
TCPClient checkClient;
TCPServer audioServer = TCPServer(3443);


void setup() {
    Serial.begin(115200);
    pinMode(MICROPHONE_PIN, INPUT);

    Spark.variable("ipAddress", myIpAddress, STRING);
    Particle.function("runServo", runServo);
    IPAddress myIp = WiFi.localIP();
    sprintf(myIpAddress, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
    pinMode(PIRPin, INPUT);     
    
    audioServer.begin();
    
    lastRead = micros();

    myservo.attach(D0);   
    myservo.write(0);    
    pinMode(D7, OUTPUT);  
    
}

void loop() {
    checkClient = audioServer.available();
    if (checkClient.connected()) {
        audioClient = checkClient; 
    }
    //listen for 10 seconds, taking a sample every 125 us, send it over to the network.
    listenAndSend(100);
    
    // checks if the motion sensor output is on HIGH. If High, then there is movement. 
    if (digitalRead(PIRPin) == HIGH) {  
        //Don't do anything (there is movement)
    
    } else {
        //Means no motion. Add to counter
        counter++;
        //If it has been 30 minutes with no movement, 
        if(counter > 360){
            Particle.publish("NoMotionDected", "1"); 
        }
    
    }
    
    delay(5000);  //Delay for 5 seconds

}

void listenAndSend(int delay) {
    unsigned long startedListening = millis();
    
    while ((millis() - startedListening) < delay) {
        unsigned long time = micros();
        
        if (lastRead > time) {
            // time wrapped?
            //lets just skip a beat for now, whatever.
            lastRead = time;
        }
        
        //125 microseconds is 1/8000th of a second
        if ((time - lastRead) > 125) {
            lastRead = time;
            readMic();
        }
    }
    sendAudio();
}

 
// Callback for Timer 1
void readMic(void) {
    uint16_t value = analogRead(MICROPHONE_PIN);
    if (audioEndIdx >= AUDIO_BUFFER_MAX) {
        audioEndIdx = 0;
    }
    audioBuffer[audioEndIdx++] = value;
}

void copyAudio(uint16_t *bufferPtr) {
    //if end is after start, read from start->end
    //if end is before start, then we wrapped, read from start->max, 0->end
    
    int endSnapshotIdx = audioEndIdx;
    bool wrapped = endSnapshotIdx < audioStartIdx;
    int endIdx = (wrapped) ? AUDIO_BUFFER_MAX : endSnapshotIdx;
    int c = 0;
    
    for(int i=audioStartIdx;i<endIdx;i++) {
        bufferPtr[c++] = audioBuffer[i];
    }
    
    if (wrapped) {
        for(int i=0;i<endSnapshotIdx;i++) {
            bufferPtr[c++] = audioBuffer[i];
        }
    }
    
    audioStartIdx = audioEndIdx;
    
    if (c < AUDIO_BUFFER_MAX) {
        bufferPtr[c] = -1;
    }
}

// Callback for Timer 1
void sendAudio(void) {
    copyAudio(txBuffer);
    
    int i=0;
    uint16_t val = 0;
        
    if (audioClient.connected()) {
       write_socket(audioClient, txBuffer);
    }
    else {
        while( (val = txBuffer[i++]) < 65535 ) {
            Serial.print(val);
            Serial.print(',');
        }
        Serial.println("DONE");
    }
}

void write_socket(TCPClient socket, uint16_t *buffer) {
    int i=0;
    uint16_t val = 0;
    
    int tcpIdx = 0;
    uint8_t tcpBuffer[1024];
    
    while( (val = buffer[i++]) < 65535 ) {
        if ((tcpIdx+1) >= 1024) {
            socket.write(tcpBuffer, tcpIdx);
            tcpIdx = 0;
        }
        
        tcpBuffer[tcpIdx] = val & 0xff;
        tcpBuffer[tcpIdx+1] = (val >> 8);
        tcpIdx += 2;
    }

    if (tcpIdx > 0) {
        socket.write(tcpBuffer, tcpIdx);
    }
}

int runServo(String posValue)   
{   
    if(posValue == "Open"){ 
    digitalWrite(D7, HIGH); 
    delay(100);             
    myservo.write(180);      
    digitalWrite(D7, LOW);  
    return 1;           
    }else{
        myservo.write(0);
    }
    return 0;
    
}