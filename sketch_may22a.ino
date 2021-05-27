//TODO at lines:
//304, 424
//
//

#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// define pins in use
#define BlueLED 27
#define GreenLED 16
#define RedLED 17
#define RelayPin 5
#define WhiteButton 35
#define YellowButton 34
#define RedButton 36
#define button4 39 // not in use at the moment
#define SENSOR 14
#define SUN 13
#define MOON 12

// create variabels
float tempCelsius;

int tempLOW = 24; // set lowest temp, below this BlueLED will light up
int tempHIGH = 28; // set highest temp, above this RedLED will light up

int menu = 0; // set menu to 0 initially

int SunBrightness = 0; // sets minimum brightness of sun light
int SunFadeamount = 5; // amount that the sun light will fade, max 255
int SunFadeSpeed = 20000; // how fast the sun light will fade

int maxMoonBrightness = 238; // need to check how bright the moon should be

int MoonFadeamount = 17; // this may change due to maxMoonBrightness

int MoonBrightness = 0; // sets minimum brightness of the moon light
int moonDay = 0;  // used to set how much moon light to begin with, 1 = no moon, 14 == full moon
int MoonPhase = 0; // used to check if moon brightness is going up or down
int MoonLight; // used to get the right amount of brightness when moon down
int MoonFadeSpeed = 5000; // how fast the moon light will fade

// set initial time to 08:00 for sunrise
int StartSunriseHour = 8, StartSunriseMin = 0 ;
// set initial time to 12:00 for Plant light to turn off during day
int PlantLightOFFhour = 12, PlantLightOFFmin = 0;
// set initial time to 16:00 for Plant light to turn on during day
int PlantLightONhour = 16, PlantLightONmin = 0;
// set initial time to 19:30 for sunrise
int StartSunsetHour = 19, StartSunsetMin = 30;

// for creating "°" for LCD display
byte tempDot[] = {
  B01110,
  B01010,
  B01110,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

// define components libaries
OneWire oneWire(SENSOR);
DallasTemperature sensors(&oneWire);
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// variables for Light Channels
int freq = 4000;
int SunChannel = 0;
int MoonChannel = 1;
int resolution = 8;

int ShowMenu = 0;

bool LCDbacklight = false;

unsigned long pressedAt;
unsigned long interval = 1;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  if (! rtc.begin()) {
    Serial.println("Didn't find rtc");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // set date and time to when uploaded
  lcd.begin(); // start LCD

  // setup for lights
  ledcSetup(SunChannel, freq, resolution);
  ledcSetup(MoonChannel, freq, resolution);
  ledcAttachPin(SUN, SunChannel);
  ledcAttachPin(MOON, MoonChannel);

  lcd.createChar(0, tempDot); // creating "°" to be used in program

  // define INPUT/OUTPUT pins
  pinMode(WhiteButton, INPUT);
  pinMode(YellowButton, INPUT);
  pinMode(RedButton, INPUT);
  pinMode(button4, INPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(RedLED, OUTPUT);
  pinMode(RelayPin, OUTPUT);

  digitalWrite(RelayPin, LOW);
}

void loop() {
  DateTime now = rtc.now(); // get time from RTC
  readTemp(); // get temp from temperature sensor
  // monitor if WhiteButton is pressed, if pressed add one to menu variable

  if (digitalRead(WhiteButton) == 1) {
    ShowMenu++;
  }
  if (ShowMenu == 0) {
    DisplayTime();
    if (now.minute() == pressedAt + interval) {
      LCDbacklight = false;
      pressedAt = 0;
    }
    // turn LCD backlight on/off
    if (digitalRead(YellowButton) == 1) {
      pressedAt = now.minute();
      Serial.println(pressedAt);
      if (LCDbacklight == false) {
      lcd.backlight();
      LCDbacklight = true;
    } else if (LCDbacklight == true) {
      lcd.noBacklight();
      LCDbacklight = false;
    }
 } 

  } else {
    Menu();
  }

  // for printing to monitor in Arduino IDE for testing
  //  Serial.println(btnState);
  //  Serial.print("Menu: ");
  //  Serial.println(menu);
  //  Serial.print(now.hour(), DEC);
  //  Serial.print(":");
  //  Serial.print(now.minute(), DEC);
  //  Serial.print(":");
  //  Serial.println(now.second(), DEC);
  //  printTemp();

  // checking time for doing stuff
  if (now.hour() == StartSunriseHour && now.minute() == StartSunriseMin) {
    Sunrise();
  }
  if (now.hour() == PlantLightOFFhour && now.minute() == PlantLightOFFmin) {
    PlantLightOff();
  }
  if (now.hour() == PlantLightONhour && now.minute() == PlantLightONmin) {
    PlantLightOn();
  }
  if (now.hour() == StartSunsetHour && now.minute() == StartSunsetMin) {
    Sunset();
  }
  delay(50);
}

// turns on Plant lights
void PlantLightOn() {
  digitalWrite(RelayPin, HIGH);
}
// turns off Plant lights
void PlantLightOff() {
  digitalWrite(RelayPin, LOW);
}

void Sunrise() {
  // for printing to monitor in Arduino IDE during testing
  //  Serial.println("Sunrise ");
  //  Serial.print("MOON Brightness: ");
  //  Serial.println(MoonBrightness);

  while (SunBrightness < 255) {
    SunBrightness += SunFadeamount;
    ledcWrite(SunChannel, SunBrightness);
    delay(SunFadeSpeed);

    // for printing to monitor in Arduino IDE during testing
    //    Serial.print(".");

    DisplayTime(); // for updating LCD during sunrise
  }

  // for printing to monitor in Arduino IDE during testing
  //  Serial.println("Done");
  //  Serial.print("Moon phase: ");
  //  Serial.println(MoonPhase);

  MoonDown();

  // If moon not been full, add brightness. if moon been full, subtract brightness
  if (MoonPhase == 0) {
    MoonBrightness += MoonFadeamount;
    if (MoonBrightness == maxMoonBrightness) {
      MoonPhase = 1; // Full moon has occured
    }
  } else {
    MoonBrightness -= MoonFadeamount;
    if (MoonBrightness == 0) {
      MoonPhase = 0; // no moon has occured
    }
  }
  PlantLightOn(); // turns on Plant lights
}

void Sunset() {
  PlantLightOff(); // turns of plant lights

  // for printing to monitor in Arduino IDE during testing
  //  Serial.println("Sunset ");

  MoonUp();

  while (SunBrightness > 0) {
    SunBrightness -= SunFadeamount;
    ledcWrite(SunChannel, SunBrightness);
    delay(SunFadeSpeed);

    // for printing to monitor in Arduino IDE during testing
    //    Serial.print(".");

    DisplayTime(); // for updating LCD during sunrise
  }

  // for printing to monitor in Arduino IDE during testing
  //  Serial.println("Done");
  //  Serial.println("Night");
}

void MoonUp() {
  MoonLight = 0; // so that moon begings off

  // for printing to monitor in Arduino IDE during testing
  //  Serial.print("Moon rise");

  while (MoonLight < MoonBrightness) {
    MoonLight ++;
    ledcWrite(MoonChannel, MoonLight);
    delay(MoonFadeSpeed);

    // for printing to monitor in Arduino IDE during testing
    //    Serial.print(".");

    DisplayTime(); // for updating LCD during sunrise
  }

  // for printing to monitor in Arduino IDE during testing
  //  Serial.print(" Done. Moon brightness: ");
  //  Serial.println(MoonBrightness);
  //  Serial.println("Done");
}

void MoonDown() {
  MoonLight = MoonBrightness; // so moon begings at right brightness

  // for printing to monitor in Arduino IDE during testing
  //  Serial.print("Moon set");
  while (MoonLight > 0) {
    MoonLight --;
    ledcWrite(MoonChannel, MoonLight);
    delay(MoonFadeSpeed);

    // for printing to monitor in Arduino IDE during testing
    //    Serial.print(".");

    DisplayTime(); // for updating LCD during sunrise
  }

  // for printing to monitor in Arduino IDE during testing
  //  Serial.print(" Done");
  //  Serial.print(" Moon phase: ");
  //  Serial.println(MoonPhase);
  //  Serial.println("Day");
}

void readTemp() {
  // getting temperature from sensor
  sensors.requestTemperatures();
  tempCelsius = sensors.getTempCByIndex(0);

  // if temp below lowest tewmperature, BlueLED will turn on
  if (tempCelsius < tempLOW) {
    digitalWrite(BlueLED, HIGH);
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, LOW);
    // if temperature is between lowest and highest, GreenLED will turn on
  } else if (tempCelsius >= tempLOW && tempCelsius <= tempHIGH) {
    digitalWrite(BlueLED, LOW);
    digitalWrite(GreenLED, HIGH);
    digitalWrite(RedLED, LOW);
    // if temp below highest tewmperature, RedLED will turn on
  } else if (tempCelsius > tempHIGH) {
    digitalWrite(BlueLED, LOW);
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, HIGH);
  }
}

// for printing temperature to monitor in Arduino IDE during testing, NOT IN USE
//void printTemp () {
//  Serial.print("Temperature: ");
//  Serial.print(tempCelsius);    // print the temperature in Celsius
//  Serial.println("°C");
//}

void DisplayTime() {
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);

  // adding leading 0 if under 10
  if (now.hour() < 10)
    lcd.print("0");
  lcd.print(now.hour());
  lcd.print(":");

  // adding leading 0 if under 10
  if (now.minute() < 10)
    lcd.print("0");
  lcd.print(now.minute());
  lcd.setCursor(9, 0);

  readTemp(); // get temperature
  DisplayTemp(); // update temperature on LCD
  lcd.setCursor(0, 1);

  // adding leading 0 if under 10
  if (StartSunriseHour < 10)
    lcd.print("0");
  lcd.print(StartSunriseHour);
  lcd.print(":");

  // adding leading 0 if under 10
  if (StartSunriseMin < 10)
    lcd.print("0");
  lcd.print(StartSunriseMin);
  lcd.setCursor(6, 1);

  // adding leading 0 if under 10
  if (StartSunsetHour < 10)
    lcd.print("0");
  lcd.print(StartSunsetHour);
  lcd.print(":");
  lcd.print(StartSunsetMin);

  // To set charaters to the right on LCD
  if (MoonBrightness < 10) {
    lcd.setCursor(15, 1);
  } else if (MoonBrightness >= 10 && MoonBrightness < 100) {
    lcd.setCursor(14, 1);
  } else {
    lcd.setCursor(13, 1);
  }
  lcd.print(MoonBrightness);
}

void DisplayTemp() {
  lcd.print(tempCelsius);
  lcd.write(0);
  lcd.print("C");
}

// set amount of moon brightness to begin with, 0 = no moon, 14 = Full moon
void setMoonPhase() {
  lcd.clear();
  lcd.print("Set moon day: ");
  if (moonDay < 10) {
    lcd.setCursor(15, 0);
  } else if (moonDay >= 10) {
    lcd.setCursor(14, 0);
  }
  lcd.print(moonDay);
  if (digitalRead(YellowButton) == 1) {
    if (moonDay == 14) {
      moonDay = 0;
    } else {
      moonDay++;
    }
  }
  MoonBrightness = MoonFadeamount * moonDay; // set MoonBrigthness by day
  ledcWrite(MoonChannel, MoonBrightness);
  lcd.setCursor(0, 1);
  lcd.print("Brightness:");
  if (MoonBrightness < 10) {
    lcd.setCursor(15, 1);
  } else if (MoonBrightness >= 10 && MoonBrightness < 100) {
    lcd.setCursor(14, 1);
  } else if (MoonBrightness >= 100) {
    lcd.setCursor(13, 1);
  }
  lcd.print(MoonBrightness);
}

// set the maximum brightness of moon light, might be to bright for small aquarium
void setMoonBrightness() {
  lcd.clear();
  lcd.print("Moon light:");
  if (MoonBrightness < 10) {
    lcd.setCursor(15, 0);
  } else if (MoonBrightness >= 10 && MoonBrightness < 100) {
    lcd.setCursor(14, 0);
  } else if (MoonBrightness >= 100) {
    lcd.setCursor(13, 0);
  }
  lcd.print(MoonBrightness);
  lcd.setCursor(2, 1);
  lcd.print("Moon max:");
  if (maxMoonBrightness < 10) {
    lcd.setCursor(15, 1);
  } else if (maxMoonBrightness >= 10 && maxMoonBrightness < 100) {
    lcd.setCursor(14, 1);
  } else if (maxMoonBrightness >= 100) {
    lcd.setCursor(13, 1);
  }
  lcd.print(maxMoonBrightness);

  if (digitalRead(YellowButton) == 1) {
    if (maxMoonBrightness == 238) {
      maxMoonBrightness = 0;
    } else {
      maxMoonBrightness += MoonFadeamount;
    }
    if (MoonBrightness >= maxMoonBrightness) {
      MoonBrightness = maxMoonBrightness;
    }
  }
  // for lighting the moon light to see how bright maximum is
  if (digitalRead(RedButton) == 1) {
    ledcWrite(MoonChannel, maxMoonBrightness);
    delay(2000);
    ledcWrite(MoonChannel, 0);
  }
}

// creating menu
void Menu() {
  if (digitalRead(WhiteButton) == 1) {
    menu++;
  }
  if (menu > 10) {
    menu = 0;
  }
  switch (menu) {
    case 1:
      SetSunriseTime();
      break;
    case 2:
      SetSunsetTime();
      break;
    case 3:
      SetPlantLightOffTime();
      break;
    case 4:
      SetPlantLightOnTime();
      break;
    case 5:
      adjustClock();
      break;
    case 6:
      setMinTemperature();
      break;
    case 7:
      setMaxTemperature();
      break;
    case 8:
      setMoonBrightness();
      break;
    case 9:
      setMoonPhase();
      break;
    case 10:
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("TO EXIT MENU");
      lcd.setCursor(0, 1);
      lcd.print("Press Red Button");
      if (digitalRead(RedButton) == 1 ) {
        lcd.clear();
        ShowMenu = 0;
      }
      break;
  }
}

// for adjusting time
void adjustClock() {
  DateTime now = rtc.now();
  // declaring variables for date/time
  int y = now.year();
  int m = now.month();
  int d = now.day();
  int h = now.hour();
  int mi = now.minute();
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Adjust Clock: ");
  lcd.setCursor(5, 1);
  if (now.hour() < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(now.minute());
  if (digitalRead(YellowButton) == 1) {
    if (h == 23) {
      h = 0;    // for going to 0, instead of 24 after 23
    } else {
      h++;
    }
  }
  if (digitalRead(RedButton) == 1) {
    if (mi == 59) {
      mi = 0;    // for going to 0, instead of 60 after 55
      h++; // add 1 hour
    } else {
      mi++;
    }
  }
  // send to RTC
  rtc.adjust(DateTime(y, m, d, h, mi, 0));
}

void SetSunriseTime() {
  lcd.clear();
  lcd.print("Start sunrise at ");
  lcd.setCursor(5, 1);
  if (StartSunriseHour < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(StartSunriseHour);
  lcd.print(":");
  if (StartSunriseMin < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(StartSunriseMin);
  if (digitalRead(YellowButton) == 1) {
    if (StartSunriseHour == 23) {
      StartSunriseHour = 0;    // for going to 0, instead of 24 after 23
    } else {
      StartSunriseHour++;
    }
  }
  if (digitalRead(RedButton) == 1) {
    if (StartSunriseMin == 55) {
      StartSunriseMin = 0;    // for going to 0, instead of 60 after 55
    } else {
      StartSunriseMin += 5;
    }
  }
}
void SetPlantLightOffTime() {
  lcd.clear();
  lcd.print("Plant light off: ");
  lcd.setCursor(5, 1);
  if (PlantLightOFFhour < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(PlantLightOFFhour);
  lcd.print(":");
  if (PlantLightOFFmin < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(PlantLightOFFmin);

  if (digitalRead(YellowButton) == 1) {
    if (PlantLightOFFhour == 23) {
      PlantLightOFFhour = 0;    // for going to 0, instead of 24 after 23
    } else {
      PlantLightOFFhour++;
    }
    PlantLightONhour = PlantLightOFFhour;
  }
  if (digitalRead(RedButton) == 1) {
    if (PlantLightOFFmin == 55) {
      PlantLightOFFmin = 0;    // for going to 0, instead of 60 after 55
    } else {
      PlantLightOFFmin += 5;
    }
  }
}
void SetPlantLightOnTime() {
  lcd.clear();
  lcd.print("Plant light on: ");
  lcd.setCursor(5, 1);
  if (PlantLightONhour < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(PlantLightONhour);
  lcd.print(":");
  if (PlantLightONmin < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(PlantLightONmin);
  if (digitalRead(YellowButton) == 1) {
    if (PlantLightONhour == 23) {
      PlantLightONhour = 0;    // for going to 0, instead of 24 after 23
    } else {
      PlantLightONhour++;
    }
  }
  if (digitalRead(RedButton) == 1) {
    if (PlantLightONmin == 55) {
      PlantLightONmin = 0;    // for going to 0, instead of 60 after 55
    } else {
      PlantLightONmin += 5;
    }
  }
}

void SetSunsetTime() {
  lcd.clear();
  lcd.print("Start sunset at ");
  lcd.setCursor(5, 1);
  if (StartSunsetHour < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(StartSunsetHour);
  lcd.print(":");
  if (StartSunsetMin < 10)
    lcd.print("0");  // adding leading 0 if under 10
  lcd.print(StartSunsetMin);
  if (digitalRead(YellowButton) == 1) {
    if (StartSunsetHour == 23) {
      StartSunsetHour = 0;    // for going to 0, instead of 24 after 23
    } else {
      StartSunsetHour++;
    }
  }
  if (digitalRead(RedButton) == 1) {
    if (StartSunsetMin == 55) {
      StartSunsetMin = 0;    // for going to 0, instead of 60 after 55
    } else {
      StartSunsetMin += 5;
    }
  }
}

void setMinTemperature() {
  lcd.clear();
  lcd.print("Set Minimum temp");
  lcd.setCursor(6, 1);
  lcd.print(tempLOW);
  lcd.write(0); // print "°" to LCD
  lcd.print("C");
  if (digitalRead(YellowButton) == 1) {
    tempLOW++;
  }
  if (digitalRead(RedButton) == 1) {
    tempLOW--;
  }
}

void setMaxTemperature() {
  lcd.clear();
  lcd.print("Set Maximum temp");
  lcd.setCursor(6, 1);
  lcd.print(tempHIGH);
  lcd.write(0); // print "°" to LCD
  lcd.print("C");
  if (digitalRead(YellowButton) == 1) {
    tempHIGH++;
  }
  if (digitalRead(RedButton) == 1) {
    tempHIGH--;
  }
}