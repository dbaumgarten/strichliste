#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define NO_BUTTON 0
#define BTN_UP 1
#define BTN_DOWN 2
#define BTN_LEFT 3
#define BTN_RIGHT 4
#define BTN_SELECT 5

#define USER_LIST_ADDR 1
#define USER_TIMEOUT_MS 5000
#define CARD_ID_LENGTH 8


// TODO:
// scanner on/off
// gehäuse
// sanity checks
// friendly name
// übersichtsmodus
// lcd lesbarkeit



typedef struct User {
  char cardid[CARD_ID_LENGTH];
  int8_t credit;
} User;

User user = { "00000000",0};
bool userscreen = false;
unsigned long timeLastAction = 0;

SoftwareSerial mySerial (2, 3);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  mySerial.begin(9600);
  lcd.begin(16, 2);
  pinMode(A0, INPUT);
  Serial.print("INIT");

  // for (int i = 0; i < EEPROM.length(); i++){
  // EEPROM.update(i,0);
  // }

  idleScreen();
}


void loop() {

  if (mySerial.available()){
    String read = mySerial.readStringUntil('\n');

    if (read.length() == CARD_ID_LENGTH + 1){
        char cardid[CARD_ID_LENGTH];
        memcpy(cardid, read.c_str(), CARD_ID_LENGTH);

        bool found = readUserFromStorage(cardid, &user);
        if (!found){
          // user does not exist. Create a new DB entry
          memcpy(user.cardid, cardid, CARD_ID_LENGTH);
          user.credit = 0;
          writeUserToStorage(user);
        }

        statusScreen(user);
        userscreen = true;
        timeLastAction = millis();
    } else if (userscreen) {
      changeCurrentUserCredit(-1);
    }
  }

  
  while (userscreen){
    int button = getButton();
    if (button == BTN_UP){
      changeCurrentUserCredit(1);
    }
    else if (button == BTN_DOWN){
     changeCurrentUserCredit(-1);
    }
    if(mySerial.available()){
      return;
    }
    if((timeLastAction + USER_TIMEOUT_MS) < millis()){
      userscreen = false;
      idleScreen();
      return;
    }
  }
}

void changeCurrentUserCredit(int delta){
  user.credit += delta;
  statusScreen(user);
  writeUserToStorage(user);
  delay(500);
  timeLastAction = millis();
}


bool readUserFromStorage(char cardid[CARD_ID_LENGTH], User *out){
  int len = EEPROM.read(0);
  for (int i = 0; i < len; i++){
    EEPROM.get(USER_LIST_ADDR+((sizeof(User)*i)), *out);
    if (isCardIDEqual((*out).cardid, cardid)){
      return true;
    }
  }
  return false;
}

void writeUserToStorage(User user){
  int len = EEPROM.read(0);
  int addr = USER_LIST_ADDR;
  bool found = false;
  for (int i = 0; i < len; i++){
    User read = {};
    EEPROM.get(addr, read);
    if (isCardIDEqual(user.cardid,read.cardid)){
      found = true;
      break;
    }
    addr=USER_LIST_ADDR+((sizeof(User)*(i+1)));
  }
  EEPROM.put(addr,user);
  if (!found){
    EEPROM.update(0,len+1);
  }
}

bool isCardIDEqual(char cardid1[CARD_ID_LENGTH], char cardid2[CARD_ID_LENGTH]){
  return memcmp(cardid1, cardid2, CARD_ID_LENGTH) == 0;
}

void statusScreen(User user){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write(user.cardid, CARD_ID_LENGTH);
  lcd.setCursor(0,1);
  lcd.write("Guthaben: ");
  char buf[10];
  itoa(user.credit, buf, 10);
  lcd.write(buf);
}

void idleScreen(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write("Karte Scannen");
}

int getButton(){
  int value = analogRead(A0);
  if (value > 1020){
    return NO_BUTTON;
  }
  if (value > 700){
    return BTN_SELECT;
  }
  if (value > 450){
    return BTN_LEFT;
  }
  if (value > 300){
    return BTN_DOWN;
  }
  if (value > 120){
    return BTN_UP;
  }
  return BTN_RIGHT;
}
