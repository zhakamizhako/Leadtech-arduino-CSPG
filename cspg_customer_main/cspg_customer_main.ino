#define VERSION "v0.001b" // Software Version
#define DEV_MESSAGE "Roland Kim Andre Solon"
#define BUILD_NUMBER "Build 0001 06/16/19"
#define UID "DEV-0000-0001"
//Leadtech
//Arduino sketch for CSPG customer module
//June 2019
//Author: trosh, kim

//snippets used from
//DS1302 RTC from (https://playground.arduino.cc/Main/DS1302RTC/)
//OLED graphics lib from U8glib code (http://code.google.com/p/u8glib/)

#include <DS1302RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <U8glib.h>
#include <SPI.h>
#include <MFRC522.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
// Init the DS1302
// Set pins:  CE, IO,CLK
DS1302RTC RTC(5, 6, 7);

char strTime[9]; //Lateral time string variable to display
char strDate[15]; //Lateral time string variable to display

//u8glib
U8GLIB_SSD1306_128X64 u8g(18, 20, 26, 24);  // SW SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9

// Init the LCD
//   initialize the library with the numbers of the interface pins
//            lcd(RS,  E, d4, d5, d6, d7)
//LiquidCrystal lcd(8,   9,  4,  5,  6,  7);

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  u8g.drawStr( 0, 22, "This is CSPG");
}

void draw_str(const char *s) {
  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 22, s);
  } while( u8g.nextPage() );
}

void draw_str(const char * strLine1, const char * strLine2) {
  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 22, strLine1);
    u8g.drawStr( 0, 44, strLine2);
  } while( u8g.nextPage() );
}

void draw_str(unsigned int x, unsigned int y, const char *s) {
  u8g.firstPage();  
  do {
    u8g.drawStr( x, y, s);
  } while( u8g.nextPage() );
}

void setup()
{
  pinMode(8, OUTPUT);
  Serial.begin(115200);
  Serial.println("[SYS][BOOT]CPSG System");
  Serial.println(BUILD_NUMBER);
  Serial.println(VERSION);
  Serial.println(DEV_MESSAGE);
  Serial.println(UID);
  //reset OLED display
  digitalWrite(8, LOW);
  digitalWrite(8, HIGH);
  digitalWrite(2, LOW);
//  readRAM( uint8_t *p);
//  writeRAM(uint8_t *p);
  
  //u8glib setup
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
  //set font
  u8g.setFont(u8g_font_unifont);

  // Check clock oscillation  
  if (RTC.haltRTC())
    draw_str("Warning! RTC has been reset!");
  else
    draw_str("RTC is running.");
    
  delay(1000);
  
  // Check write-protection
  if (RTC.writeEN())
    draw_str("Write allowed");
  else
    draw_str("Write protected");

  delay ( 1000 );

  // Setup Time library
  draw_str("RTC Sync");
  setTime(00,52,30,2,6,2019); //hr,min,sec,day,mnth,yr
  setSyncProvider(RTC.get); // the function to get the time from the RTC
  
  if(timeStatus() == timeSet)
    draw_str("RTC Synced");
  else
    draw_str("RTC FAILED");

  delay ( 1000 );

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
}

void loop()
{
  // Display time centered on the upper line
  GetTimeInStr(strTime, hour(), minute(), second());
  GetDateInStr(strDate, weekday(), month(), day(), year());
  draw_str(strTime, strDate);
//  Serial.println(strTime);
//  Serial.println(strDate);
  if (Serial.available()) { // Time syncer via Serial monitor
    Serial.println("[SYS]Serial Monitor Activity");
    processSyncMessage();
  }
  // Warning!
  /*if(timeStatus() != timeSet) {
    //lcd.setCursor(0, 1);
    //lcd.print(F("RTC ERROR: SYNC!"));
    draw_str("RTC ERROR: SYNC!");
  }*/

  //Read RFID
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  draw_str("[RFID]Reading IDCARD...");
  delay(1000);
//  delay ( 1000 ); // Wait approx 1 sec
}

//Format time in a nice string to be displayed
//Parameters: (string container, hour, min, sec)
void GetTimeInStr(char * vString, int vHour, int vMinute, int vSecond){
  char tStr1[3];
  char tStr2[3];

  //Display always in double digit
  if(vHour < 10){
    itoa(0, tStr1, 10);
    itoa(vHour, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  else{
    itoa(vHour, tStr1, 10);
  }
  
  strcat(tStr1, ":");
  
  if(vMinute < 10){
    strcat(tStr1, "0");
    itoa(vMinute, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  else{
    itoa(vMinute, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  
  strcat(tStr1, ":");
  
  if(vSecond < 10){
    strcat(tStr1, "0");
    itoa(vSecond, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  else{
    itoa(vSecond, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  
  strcpy(vString, tStr1);
}

//Format time in a nice string to be displayed
//Parameters: (string container, weekday, month, date, year)
void GetDateInStr(char * vString, int vWeekday, int vMonth, int vDay, int vYear){
  char tStr1[4];
  char tStr2[5];

  switch(vWeekday){
    case 1: strcpy(tStr1, "Sun");
    break;

    case 2: strcpy(tStr1, "Mon");
    break;

    case 3: strcpy(tStr1, "Tue");
    break;

    case 4: strcpy(tStr1, "Wed");
    break;

    case 5: strcpy(tStr1, "Thu");
    break;

    case 6: strcpy(tStr1, "Fri");
    break;

    case 7: strcpy(tStr1, "Sat");
    break;

    default:  strcpy(tStr1, "Err");
  }
  
  strcat(tStr1, "-");
  
  itoa(vMonth, tStr2, 10);
  strcat(tStr1, tStr2);
  
  strcat(tStr1, "-");

  itoa(vDay, tStr2, 10);
  strcat(tStr1, tStr2);
  
  strcat(tStr1, "-");
  
  itoa(vYear, tStr2, 10);
  strcat(tStr1, tStr2);
    
  strcpy(vString, tStr1);
}
//Process Time via Serial Monitor function
//Should add a verifier for this to avoid cheating.
//Maybe the number of ticks should do?
void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
       Serial.println("[SYS][CLOCK] Time Set.");
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}
