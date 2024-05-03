/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2020-2022 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
////////////////////////////////////////////////////////////
//                                                        //
//    HomeSpan: A HomeKit implementation for the ESP32    //
//    ------------------------------------------------    //
//                                                        //
// Example 4: A variable-speed ceiling fan with           //
//            dimmable ceiling light                      //
//                                                        //
////////////////////////////////////////////////////////////


#include "HomeSpan.h"         // Always start by including the HomeSpan library

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Adafruit_EMC2101.h>
Adafruit_EMC2101  emc2101;
unsigned long lastMillis;
// Use dedicated hardware SPI pins
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);


struct VentFan : Service::Fan{
  Adafruit_EMC2101  emc2101;
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
  SpanCharacteristic *ventOn;                             
  SpanCharacteristic *ventSpeed;             // store a reference to the On Characteristic
  
  VentFan(Adafruit_EMC2101  emc2101, Adafruit_ST7789  tft) : Service::Fan(){       // constructor() 
    ventOn = new Characteristic::Active();   
    ventSpeed = (new Characteristic::RotationSpeed(50))->setRange(0,100,25); 
               
      // new Characteristic::RotationDirection();                        // NEW: This allows control of the Rotation Direction of the Fan
      // (new Characteristic::RotationSpeed   // NEW: This allows control of the Rotation Speed of the Fan, with an initial value of 50% and a range from 0-100 in steps of 25%
    // ventOn=new Characteristic::On();      // instantiate the On Characteristic and save it as lampPower
    this->emc2101=emc2101;     
    
    this->tft=tft;
    this->tft.init(135, 240); // Init ST7789 240x135
    this->tft.setRotation(3);
    this->tft.fillScreen(ST77XX_BLACK);        
    
    
  } // end constructor()
  void updateDisplay(){
    //   Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
    // screen 240 x 135
    tft.setTextWrap(false);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 1);
    tft.setTextColor(ST77XX_BLUE);
    tft.setTextSize(3);
    tft.println("RPM:");
    tft.setCursor(120, 1);
    tft.println("PWM:");
    
    tft.setCursor(0, 41);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.print(emc2101.getFanRPM());
    // tft.println(" C");
    // tft.print(20);
    // tft.println(" F");
    
    tft.setCursor(120, 41);
    tft.print(emc2101.getDutyCycle());
    // tft.println("%");
    tft.setCursor(0, 110);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    // tft.println(WiFi.localIP());
  }
  boolean update(){                          // update() method

    emc2101.setDutyCycle(ventSpeed->getNewVal()) ;
    this->updateDisplay();
    return(true);                            // return true to let HomeKit (and the Home App Client) know the update was successful
  
  } // end update()
  
};

void setup() {

  // Example 4 expands on the first Accessory in Example 3 by adding Characteristics to set FAN SPEED, FAN DIRECTION, and LIGHT BRIGHTNESS.
  // For ease of reading, all prior comments have been removed and new comments added to show explicit changes from the previous example.
 
  Serial.begin(115200); 
  
  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);
  
  tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  Serial.println(F("TFT Initialized"));

  // updateDisplay();

  Serial.println("Adafruit EMC2101 test!");

  // Try to initialize!
  if (!emc2101.begin(0x4C)) {
    Serial.println("Failed to find EMC2101 chip");
    while (1) { delay(10); }
  }
  Serial.println("EMC2101 Found!");

  homeSpan.begin(Category::Fans,"HomeSpan Prusa VentFan");  
  // homeSpan.setStatusPixel(13);
  new SpanAccessory();                            
  
    new Service::AccessoryInformation();                
      new Characteristic::Identify();                        

    // new Service::Fan();                             
     
    new VentFan(emc2101, tft);
  // NOTE 1: Setting the initial value of the Brightness Characteristic to 50% does not by itself cause HomeKit to turn the light on to 50% upon start-up.
  // Rather, this is governed by the initial value of the On Characteristic, which in this case happens to be set to true.  If it were set to false,
  // or left unspecified (default is false) then the LightBulb will be off at start-up.  However, it will jump to 50% brightness as soon as turned on
  // for the first time.  This same logic applies to the Active and RotationSpeed Characteristics for a Fan.

  // NOTE 2: The default range for Characteristics that support a range of values is specified in HAP Section 9.  For Brightness, the range defaults
  // to min=0%, max=100%, step=1%.  Using setRange() to change the minimum Brightness from 0% to 20% (or any non-zero value) provides for a better
  // HomeKit experience.  This is because the LightBulb power is controlled by the On Characteristic, and allowing Brightness to be as low as 0%
  // sometimes results in HomeKit turning on the LightBulb but with Brightness=0%, which is not very intuitive.  This can occur when asking Siri
  // to lower the Brightness all the way, and then turning on the LightBulb.  By setting a minumum value of 20%, HomeKit always ensures that there is
  // some Brightness value whenever the LightBulb is turned on.

} // end of setup()




//////////////////////////////////////

void loop(){
  
  homeSpan.poll();         // run HomeSpan!
  if (millis() - lastMillis >= 1*1000UL) 
  {
   lastMillis = millis();  //get ready for the next iteration
    tft.setTextWrap(false);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 1);
    tft.setTextColor(ST77XX_BLUE);
    tft.setTextSize(3);
    tft.println("RPM:");
    tft.setCursor(120, 1);
    tft.println("PWM:");
    
    tft.setCursor(0, 41);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.print(emc2101.getFanRPM());
    // tft.println(" C");
    // tft.print(20);
    // tft.println(" F");
    
    tft.setCursor(120, 41);
    tft.print(emc2101.getDutyCycle());
    // tft.println("%");
    tft.setCursor(0, 110);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
  
  }
  
} // end of loop()
