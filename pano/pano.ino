#include <Servo.h>
#include <ESP8266WiFi.h>
#include <Servo.h>

/*********************************
 * YOUR SETTINGS *
 *********************************/
const char * ssid = "hero"; //Your Wifi name (SSID)
const char * password = "xxxxxxxx"; //Your WiFi password
Servo myservo;  // create servo object to control a servo

int potpin = 0;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog pin

const char * host = "10.5.5.9";
const int httpPort = 80;
int delaylong = 500;
int delaylonger = 2000;
#define STEPS (2038) // the number of steps in one revolution of your motor (28BYJ-48)

class Stepper{
  public:
    Stepper(int rev, int p1, int p2, int p3, int p4) {
      this->p1=p1;
      this->p2=p2;
      this->p3=p3;
      this->p4=p4;
      this->rev=rev;
      c=0;
      pinMode(p1, OUTPUT);
      pinMode(p2, OUTPUT);
      pinMode(p3, OUTPUT);
      pinMode(p4, OUTPUT);
    }
    void setSpeed(int whatSpeed) {
    }
    void step(int number_of_steps) {
      unsigned char pinOuts[] = {0b0011,0b0110,0b1100,0b1001};
      for(int i=0;i<abs(number_of_steps);i++) {
        if(number_of_steps<0) c++;
        else c--;
        setPins(pinOuts[c&0x3]);
        delay(5);
      }
      setPins(0);
    }
    ~Stepper(){}
  private:
    int rev,p1,p2,p3,p4;
    unsigned char c;
    void setPins(unsigned char pins) {
      digitalWrite(p1, pins&0x1);
      digitalWrite(p2, pins&0x2);
      digitalWrite(p3, pins&0x4);
      digitalWrite(p4, pins&0x8);
    }
};
Stepper myStepper(STEPS, 5, 4, 0, 2);

void led(bool l) {
  if(l) {
    digitalWrite(12, LOW);
  } else {
    digitalWrite(12, HIGH);
  }
}

void makepicture() {
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");     
      return;
  }
  String url = "/camera/SH?t="; url += password; url += "&p=%01";
  Serial.println(" Click !");
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
      "Host: " + host + "\r\n" +
      "Connection: close\r\n\r\n");
  delay(delaylonger); 
}

void setPhotoMode() {
    String url = "/camera/CM?t="; url += password; url += "&p=%01";

    WiFiClient client;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
        "Host: " + host + "\r\n" +
        "Connection: close\r\n\r\n");
    delay(10);
    Serial.println();
    Serial.println("closing connection");
}

#include <ESP8266HTTPClient.h>


String parseSetting(const char*settings, const char *wanted) {
      const char *shutterTime = strstr(settings, wanted);
      if(shutterTime==NULL) return "";
      
      shutterTime = strchr(shutterTime, ':') + 1;
      const char *shutterTimeend = strchr(shutterTime, ',');
      if(shutterTimeend==NULL) return "";
      
      char b[20];
      memset(b,0,20);
      memcpy(b, shutterTime, shutterTimeend-shutterTime);
      return b;
}

int getShutterTime() {
    int ret = 2;
    String url = String("http://") + String(host) + "/gp/gpControl/status";
        Serial.println("GetShuttern-START");
        HTTPClient http;
      http.begin(url); //HTTP

      int httpCode = http.GET();

      // httpCode will be negative on error
      if(httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);

          // file found at server
          if(httpCode == HTTP_CODE_OK) {
              String payload = http.getString();

              const char *settings = strstr(payload.c_str(), "settings");
              String shutterTime = parseSetting(settings, "\"19\":");
              String subMode = parseSetting(settings, "\"69\":");
              
              Serial.println(shutterTime);
              Serial.println(subMode);

              if(subMode=="2") {  //NIGTH MODE
                  if(shutterTime=="0") ret = 5;
                  if(shutterTime=="1") ret = 2;
                  if(shutterTime=="2") ret = 5;
                  if(shutterTime=="3") ret = 10;
                  if(shutterTime=="4") ret = 15;
                  if(shutterTime=="5") ret = 20;
                  if(shutterTime=="6") ret = 30;
                  ret+=5;
              }
          }
      } else {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
 Serial.printf("ShutterTime %d\n",ret);
      return ret;
}

int getMac() {
    String url = String("http://") + String(host) + "/gp/gpControl";
     Serial.println("getMac-START");
        HTTPClient http;
      http.begin(url); //HTTP

      int httpCode = http.GET();
      char buf[25] = { 0 };
      // httpCode will be negative on error
      if(httpCode > 0) {
          WiFiClient * stream = http.getStreamPtr();
          // read all data from server
          int len = http.getSize();
          buf[sizeof(buf)-1] = 0;
          while (http.connected() && (len > 0 || len == -1)) {
            // get available data size
            size_t size = stream->available();
  
            if (size) {
              memcpy(buf, buf+1, sizeof(buf));
              buf[sizeof(buf)-2] = (char)(stream->read());
              
              if(strstr(buf, "\"ap_mac\"")==&buf[0]) {
                Serial.printf("MAC:%s\n", buf);
              }
            }
            delay(0);
          } 
      } else {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
      return 0;
}
#define SERVO_UP 230
#define SERVO_FRONT 80
#define SERVO_MIDLE 150
void servoMove(int angle) {
  myservo.attach(14);
  myservo.write(angle);
  delay(3000);
  myservo.detach();
}
void setup() {
    myStepper.setSpeed(5);
    //pinMode(13, INPUT); //LASKIN d7
    pinMode(13, INPUT); //BUTTON d6
    pinMode(12, OUTPUT); //LED d5

    Serial.begin(115200);

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    servoMove(SERVO_MIDLE);
}


#include <WiFiUdp.h>
WiFiUDP Udp;

void wakeupGoPro() {
      char b[] = {0xff,0xff,0xff,0xff,0xff,0xff};
      char MAC_GOPRO[] = {0xd8, 0x96, 0x85, 0x56, 0x9d, 0x8c};

      Udp.beginPacket(host, 9);
      Udp.write(b, 6);
      Udp.endPacket();
          
      for(int x=0;x<10;x++) {
          Udp.beginPacket(host, 9);
          for(int i=0;i<16;i++){
            Udp.write(MAC_GOPRO, 6);
          }
          Udp.endPacket();
      }
}

int sleepGoPro() {
    String url = String("http://") + String(host) + "/gp/gpControl/command/system/sleep";
        Serial.println("GetShuttern-START");
        HTTPClient http;
      http.begin(url); //HTTP

      int httpCode = http.GET();

      // httpCode will be negative on error
      if(httpCode > 0) {
        
      } else {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          return 1;
      }

      http.end();
      return 0;
}
void loop() {
   if (WiFi.status() != WL_CONNECTED) {
        led(true);
        delay(100);
        led(false);
        delay(100);
        Serial.print(".");
  } else {
    led(true);
  }

  
  if(!digitalRead(13)) {
    wakeupGoPro();
    for(int i=0;i<25;i++) {
      led(true);
      wakeupGoPro();
      delay(50);
      wakeupGoPro();
      led(false);
      delay(500);
    }

    int shutterTime = 0;
    if (WiFi.status() == WL_CONNECTED) {
      //setPhotoMode();
      shutterTime = getShutterTime();
    }

    
    servoMove(SERVO_UP); //ylös
      makepicture();
      delay(shutterTime*1000);
    servoMove(SERVO_FRONT); //ylös

    for(int i=0;i<8;i++) {
      led(true);
      myStepper.step(STEPS/8);
      led(false);
      delay(2000);
      if (WiFi.status() == WL_CONNECTED) {
        makepicture();
        delay(shutterTime*1000);
      }
    }
    sleepGoPro();
    myStepper.step((STEPS/8)*-8);
    
  }

  led(true); 
}
