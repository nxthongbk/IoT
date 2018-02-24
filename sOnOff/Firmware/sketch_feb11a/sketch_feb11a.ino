#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <time.h>
#include <FirebaseArduino.h>

#define PIN_LED 16 //D0
#define PIN_OUT 5 //D1 - connect to Relay
#define PIN_BUTTON 0 //D3 - for smart configure

#define LED_ON() digitalWrite(PIN_LED, HIGH)
#define LED_OFF() digitalWrite(PIN_LED, LOW)
#define LED_TOGGLE() digitalWrite(PIN_LED, digitalRead(PIN_LED) ^ 0x01)

//Defile Firebase
#define FIREBASE_HOST "sonoff-6b6ea.firebaseio.com"  
#define FIREBASE_AUTH ""

int current_on;
int current_off;

Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(PIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(PIN_LED, !state);     // set pin to the opposite state
  
}

bool in_smartconfig = false;
void enter_smartconfig()
{
  if (in_smartconfig == false) {
    in_smartconfig = true;
    ticker.attach(0.1, tick);
    WiFi.beginSmartConfig();
  }
}

bool longPress()
{
  static int lastPress = 0;
  if (millis() - lastPress > 3000 && digitalRead(PIN_BUTTON) == 0) {
    return true;
  } else if (digitalRead(PIN_BUTTON) == 1) {
    lastPress = millis();
  }
  return false;
}

void exit_smart()
{
  ticker.detach();
  LED_ON();
  in_smartconfig = false;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  pinMode(16,OUTPUT);
  pinMode(PIN_OUT,OUTPUT);
  WiFi.mode(WIFI_STA);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.print("Device ID:"+WiFi.macAddress()+"\n");
  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.print("Network connected\n");
  }
}

void loop() {
  // Check Smart Config:
   if (longPress()) {
      enter_smartconfig();
      Serial.println("Enter smartconfig");
  }

  //Runing: 
  //0 - Waiting
  //1 - Turn on
  //2 - Turn Off
  //3 - Repeat
  //4 - ........

  //Check Event----------------------------------------------
  if (WiFi.status() == WL_CONNECTED) 
  {
    exit_smart();

    switch (Firebase.getInt("device"+WiFi.macAddress()+"/running")) 
    {
      case 0:    // do nothing waiting
        
        break;
      case 1:    // Turn On
        Serial.print("Turn On\n");
        digitalWrite(PIN_OUT,HIGH);
        Firebase.setInt(("device"+WiFi.macAddress()+"/state"), 1);
        Firebase.setInt(("device"+WiFi.macAddress()+"/running"), 0);
        
        current_on = Firebase.getInt("device"+WiFi.macAddress()+"/turnon");
        current_on= current_on+1;
        Firebase.setInt(("device"+WiFi.macAddress()+"/turnon"), current_on);
        break;
      case 2:    // Turn Off
        Serial.print("Turn Off\n");
        digitalWrite(PIN_OUT,LOW);
        Firebase.setInt(("device"+WiFi.macAddress()+"/state"), 0);
        Firebase.setInt(("device"+WiFi.macAddress()+"/running"), 0);
      
        current_off = Firebase.getInt("device"+WiFi.macAddress()+"/turnoff");
        current_off= current_off+1;
        Firebase.setInt(("device"+WiFi.macAddress()+"/turnoff"), current_off);
        break;
      case 3:    // Repeat
        Serial.println("Repeat");
        int timer;
        int repeat;
        timer= Firebase.getInt("device"+WiFi.macAddress()+"/timer");
        repeat = Firebase.getInt("device"+WiFi.macAddress()+"/repeat");
        Serial.println("Timer: "+ timer);
        for (int i=0; i < repeat; i++){
          //Repeat On ----------------------------
          Firebase.setInt(("device"+WiFi.macAddress()+"/state"), 1);
          digitalWrite(PIN_OUT,HIGH);
          Serial.print("Repeat_Turn ON\n");
          delay(timer);
          //Repeat Off---------------------------
          Firebase.setInt(("device"+WiFi.macAddress()+"/state"), 0);
          digitalWrite(PIN_OUT,LOW);
          Serial.print("Repeat_Turn OFF\n");
          delay(timer);
        }
        Firebase.setInt(("device"+WiFi.macAddress()+"/running"), 0);
        current_on = Firebase.getInt("device"+WiFi.macAddress()+"/turnon");
        current_on= current_on+repeat;
        current_off = Firebase.getInt("device"+WiFi.macAddress()+"/turnoff");
        current_off= current_off+repeat;
        Firebase.setInt(("device"+WiFi.macAddress()+"/turnoff"), current_off);
        Firebase.setInt(("device"+WiFi.macAddress()+"/turnon"), current_on);
          
        break;
      case 4://Check online state
        Serial.print("Checking online:\n");
        Firebase.setInt(("device"+WiFi.macAddress()+"/online"), 1);
        Firebase.setInt(("device"+WiFi.macAddress()+"/running"), 0);
        Serial.print("-->Device online:\n");
        break;
      default:
        break;
    }
    //Check Firebase ----------------------------------------------- 
    if (Firebase.failed()) 
    {  
      Serial.print("setting /number failed:");
      Serial.print(Firebase.error());
      return;
    }
    delay(10); 
  }   
}
