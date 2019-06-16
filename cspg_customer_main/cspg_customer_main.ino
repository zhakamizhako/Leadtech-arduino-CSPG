#include <Streaming.h> // Easy Serial out

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

MFRC522::MIFARE_Key key;
constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
// Init the DS1302
// Set pins:  CE, IO,CLK
DS1302RTC RTC(5, 6, 7);
time_t timeLeft = now(); // must be a unix value
time_t t = now();
int time_rate= 60; // time rate per tap in seconds 
int load_rate = 10; // Load to decrease from card
int pin_OUTPUT = 3;
boolean active=false;
char strTime[9]; //Lateral time string variable to display
char strDate[15]; //Lateral time string variable to display

byte sector         = 1;
byte blockAddr      = 4;
byte dataBlock[]    = { 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00  
};

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
  pinMode(pin_OUTPUT, OUTPUT);
  digitalWrite(pin_OUTPUT, LOW);
  Serial.begin(115200);
  Serial.println("[SYS][BOOT]CPSG System");
  Serial.println(BUILD_NUMBER);
  Serial.println(VERSION);
  Serial.println(DEV_MESSAGE);
  Serial.println(UID);
  //reset OLED display
  digitalWrite(22, LOW);
  digitalWrite(22, HIGH);
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
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  //Add something below here that reads the CMOS RAM to read the remaining time. Most likely a UNIX timestamp, write to time_t timeLeft.
}

void loop()
{
  // Display time centered on the upper line
  if(!active){
    GetTimeInStr(strTime, hour(), minute(), second());
  GetDateInStr(strDate, weekday(), month(), day(), year());
  draw_str(strTime, strDate);
  }
  
//  Serial.println(strTime);
//  Serial.println(strDate);
  if (Serial.available()) { // Time syncer via Serial monitor
    Serial.println("[SYS]Serial Monitor Activity");
    processSyncMessage();
  }

  if(active){
    if(now()<timeLeft){
    time_t a = timeLeft - now();
//    Serial << second(a);
    
    Serial << "\nTime Left:" << timeLeft - now() << "\n";
    }
    
    if(now()>timeLeft){
      active=false;
      Serial << "\n[SYS][OUTPUT] Power deactivated.\n";
      digitalWrite(pin_OUTPUT, LOW);
    }
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
  }else {
    handleReadRFID(); // card read handler
  }
  
}

//Utils
void handleReadRFID() {
  draw_str("[RFID]Reading IDCARD...");
  
  byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);
    
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        draw_str("RFID Rejected");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    Serial << "[SYS][RFID] Reading Data";
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        draw_str("RFID Data Error");
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }

    String load = dump_byte_array(buffer, 16);
    int load_int = load.toInt();
    if(load_int - load_rate >= 0){
      int x = load_int - load_rate;
      String y = (String)x;
      Serial << "\n[SYS][RFID] Current Load: " << load << "\n";
      Serial << "[SYS][RFID] Remaining Load: " << y << "\n";
      byte tempBuffer[y.length()];
      for(int m=0;m<y.length();m++){ //copy string to buffer
        tempBuffer[m] = y[m];
      }
      for(int m=0;m<y.length();m++){ //can erase.
        Serial.println((int)tempBuffer[m]);
      }
      for(int m=0;m<16;m++) // initialize dataBlock array
        dataBlock[m]=0x00;
      for(int m=0;m<y.length();m++){ // write to dataBlock array
        dataBlock[m] =y[m];
      }
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16); // Write to RFID
       if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        draw_str("Tap card Error.");
        delay(2000);
      }else{
      String temp = "Bal.: ";
      temp += y;
      String line2 = "Loaded!";
      char screenBuffer[20]; 
      char screenBuffer2[20];
      temp.toCharArray(screenBuffer2, temp.length()+1);
      line2.toCharArray(screenBuffer,line2.length()+1);
      draw_str(screenBuffer2, screenBuffer);
      handleActive();
      delay(1000);  
      }
      
    }else{
      draw_str("Insufficient Load");
      Serial << "\n[SYS][RFID] Insufficient Load \n" << "Load Left: " << load_int << "\n";
      
      delay(1000);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void handleActive() {
  if(!active){
    active = true;
    digitalWrite(pin_OUTPUT,HIGH);
    Serial << "\n[SYS][OUTPUT]Power Active!\n";
    timeLeft = now() + time_rate;
  }else{
    timeLeft = timeLeft + time_rate;
    Serial << "\n[SYS][OUTPUT]Added\n";
  }
  //TODO: Use the CMOS RAM to store remaining time. Else, shit happens
  Serial << "\n[SYS][EPOCH]=" << timeLeft << "\n";
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

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       
       
       if(RTC.set(pctime) == 0){
        setTime(RTC.get()); // Sync Arduino clock to the time received on the serial port
        Serial.println("[SYS][CLOCK] Time Set."); 
       }
       else{
        Serial << "\n[SYS][WARN]RTC was not set!";
       }
       
     }else{
      Serial.println("[SYS][CLOCK] Time input was rejected.");
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

String dump_byte_array(byte *buffer, byte bufferSize) {
  String myString = String((char*)buffer);
  Serial.println(myString);
//  char * dest;
//  for (int cnt = 0; cnt < bufferSize; cnt++)
//  {
//    // convert byte to its ascii representation
//    sprintf(&dest[cnt * 2], "%02X", buffer[cnt]);
//  }
  
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }

    return myString;
}
