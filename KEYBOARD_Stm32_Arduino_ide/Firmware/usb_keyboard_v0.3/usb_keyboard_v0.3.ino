//caps led
#include "USBComposite.h"

/*
 * NumLock    Fn+ESC  //53(Keyboard cmd)
 * 
 * Mute       Fn+F1   E2
 * Vol-       Fn+F2   EA
 * Vol+       Fn+F3   E9
 * BACKWARD   Fn+F4   B4/B6
 * Play/Pause Fn+F5   CD
 * FORWARD    Fn+F6   B3/B5
 * 
 * LCD  PWR   Fn+F7   GPIO
 * Prjct Scrn Fn+F8   GUI+P
 * Search     Fn+F9   221
 * 
 * KBD BL 0/1 Fn+F10  GPIO
 * BRGHT+     Fn+F11  6F
 * BRGHT-     Fn+F12  70
 * 
 * WiFi Dis   Fn+PS   GPIO
 * LTE Dis    Fn+7    GPIO
 
#define KBD0 PA0
#define KBD1 PA1
#define KBD2 PA2
#define KBD3 PA3
#define KBD4 PA4
#define KBD5 PA5
#define KBD6 PA6
#define KBD7 PA7
#define KBD8 PA8
#define KBD9 PA9

#define KBD10 PB0
#define KBD11 PB1
#define KBD12 PB2
#define KBD13 PB3
#define KBD14 PB4
#define KBD15 PB5
#define KBD16 PB6
#define KBD17 PB7
#define KBD18 PB8
#define KBD19 PB9
#define KBD20 PB10
#define KBD21 PB11
#define KBD22 PB12
#define KBD23 PB13
#define KBD24 PB14
#define KBD25 PB15

#define KBD26 PC0

#define KBD29 PC3
*/

#define KBDBKL  PC6
#define WIFI    PC7
#define LTE     PC8
#define LCD     PC9

#define KEY_PS  0xCE
#define TILDE   0x7E
#define MODIFIERKEY_FN 0x8f
#define CAPS_LED 32
#define LED     45
const byte rows_max = 17;
const byte cols_max = 8;
uint8_t modifiers;
boolean Fn_pressed = HIGH;

USBHID HID;

const uint8_t reportDescription[] = {
   HID_CONSUMER_REPORT_DESCRIPTOR(),
   HID_KEYBOARD_REPORT_DESCRIPTOR()
};

HIDConsumer Consumer(HID);
HIDKeyboard Keyboard(HID);

KeyReport_t keyReport;

int normal[rows_max][cols_max] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,KEY_PS,0,0,0},
  {0,'z','a','q','1',KEY_TAB,TILDE,KEY_ESC},
  {0,'x','s','w','2',KEY_CAPS_LOCK,KEY_F1,0},
  {0,'c','d','e','3',KEY_F3,KEY_F2,KEY_F4},
  {'b','v','f','r','4','t','5','g'},
  {' ',KEY_RETURN,'\\',0,KEY_F10,KEY_BACKSPACE,KEY_F9,KEY_F5},
  {'n','m','j','u','7','y','6','h'},
  {0,',','k','i','8',']','=',KEY_F6},
  {'/',0,';','p','0','[','-','\''},
  {0,'.','l','o','9',KEY_F7,KEY_F8,0},
  {KEY_LEFT_ARROW,0,0,0,0,0,0,KEY_UP_ARROW},
  {KEY_DOWN_ARROW,0,0,0,KEY_F11,0,KEY_DELETE,0},
  {KEY_RIGHT_ARROW,0,0,0,KEY_F12,0,KEY_INSERT,0}
};
int modifier[rows_max][cols_max] = {
  {0,0,MODIFIERKEY_FN,0,0,0,0,0},
  {0,0,0,0,KEY_LEFT_GUI,0,0,0},
  {0,KEY_RIGHT_CTRL,0,0,0,0,KEY_LEFT_CTRL,0},
  {0,KEY_RIGHT_SHIFT,0,0,0,KEY_LEFT_SHIFT,0,0},
  {KEY_RIGHT_ALT,0,0,0,0,0,0,KEY_LEFT_ALT},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},   
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},    
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}, 
  {0,0,0,0,0,0,0,0}  
};
// Load the media key matrix with Fn key names at the correct row-column location. 
// A zero indicates no media key at that location.  
int media[rows_max][cols_max] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},   
  {0,0,0,0,0,0,Consumer.MUTE,0},
  {0,0,0,0,0,Consumer.VOLUME_UP,Consumer.VOLUME_DOWN,0xB6},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0x221,0xCD},   
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0xB5},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0x6F,0,0,0},
  {0,0,0,0,0x70,0,0,0}
};
// Initialize the old_key matrix with one's. 
boolean old_key[rows_max][cols_max] = {
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1}
};

int Row_IO[rows_max] = {9,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
int Col_IO[cols_max] = {1,2,3,4,5,6,7,8};
// Declare variables that will be used by functions

boolean slots_full = LOW; // Goes high when slots 1 thru 6 contain normal keys
// slot 1 thru slot 6 hold the normal key values to be sent over USB. 
int slot1 = 0; //value of 0 means the slot is empty and can be used.  
int slot2 = 0; 
int slot3 = 0; 
int slot4 = 0; 
int slot5 = 0; 
int slot6 = 0;
//
int mod_shift_l = 0; // These variables are sent over USB as modifier keys.
int mod_shift_r = 0; // Each is either set to 0 or MODIFIER_ ... 
int mod_ctrl_l = 0;   
int mod_ctrl_r = 0;
int mod_alt_l = 0;
int mod_alt_r = 0;
int mod_gui = 0;

boolean num_lock = LOW;
boolean caps_lock=LOW;
//

void setup(){
  afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);   // To use PB3 as GPIO
 
  pinMode(CAPS_LED, OUTPUT); //CAPS LED+
  pinMode(PC3, OUTPUT); //CAPS LED-
  digitalWrite(PC3, LOW);
  go_0(LTE);
  go_0(LCD);
  go_0(KBDBKL);
  go_0(WIFI);
  pinMode(LED, OUTPUT);
  
  go_1(LED);
  go_1(CAPS_LED);
  delay(200);
  go_0(LED);
  go_0(CAPS_LED);

  for (int a = 0; a < cols_max; a++) {  // loop thru all column pins 
    go_pu(Col_IO[a]); // set each column pin as an input with a pullup
  }
  for (int b = 0; b < rows_max; b++) {  // loop thru all row pins 
    go_z(Row_IO[b]); // set each row pin as a floating output
  }  
  HID.begin(reportDescription, sizeof(reportDescription));
  delay(500);
}

// Function to set a pin to high impedance (acts like open drain output)
void go_z(int pin)
{
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
}
//
// Function to set a pin as an input with a pullup
void go_pu(int pin)
{
  pinMode(pin, INPUT_PULLUP);
  digitalWrite(pin, HIGH);
}
//
// Function to send a pin to a logic low
void go_0(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}
//
// Function to send a pin to a logic high
void go_1(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
}
//
// Function to load the modifier key name into the appropriate mod variable
void load_mod(int m_key) {
  if (m_key == KEY_LEFT_SHIFT)  {
    mod_shift_l = m_key;
    Keyboard.press(m_key);
  }
  else if (m_key == KEY_RIGHT_SHIFT)  {
    mod_shift_r = m_key;
    Keyboard.press(m_key);
  }
  else if (m_key == KEY_LEFT_CTRL)  {
    mod_ctrl_l = m_key;
    Keyboard.press(m_key);
  }
  else if (m_key == KEY_RIGHT_CTRL)  {
    mod_ctrl_r = m_key;
    Keyboard.press(m_key);
  }
  else if (m_key == KEY_LEFT_ALT)  {
    mod_alt_l = m_key;
    Keyboard.press(m_key);
  }
  else if (m_key == KEY_RIGHT_ALT)  {
    mod_alt_r = m_key;
    Keyboard.press(m_key);
  }
  else if (m_key == KEY_LEFT_GUI)  {
    mod_gui = m_key;
    Keyboard.press(m_key);
  }
}
//
// Function to load 0 into the appropriate mod variable
void clear_mod(int m_key) {
  if (m_key == KEY_LEFT_SHIFT)  {
    mod_shift_l = 0;
    Keyboard.release(m_key);
  }
  else if (m_key == KEY_RIGHT_SHIFT)  {
    mod_shift_r = 0;
    Keyboard.release(m_key);
  }
  else if (m_key == KEY_LEFT_CTRL)  {
    mod_ctrl_l = 0;
    Keyboard.release(m_key);
  }
  else if (m_key == KEY_RIGHT_CTRL)  {
    mod_ctrl_r = 0;
    Keyboard.release(m_key);
  }
  else if (m_key == KEY_LEFT_ALT)  {
    mod_alt_l = 0;
    Keyboard.release(m_key);
  }
  else if (m_key == KEY_RIGHT_ALT)  {
    mod_alt_r = 0;
    Keyboard.release(m_key);
  }
  else if (m_key == KEY_LEFT_GUI)  {
    mod_gui = 0;
    Keyboard.release(m_key);
  }
}
//
// Function to load the key name into the first available slot
void load_slot(int key) {
  //uint8_t k = Keyboard.getKeyCode(key, &modifiers);
  if (!slot1)  {
    slot1 = key;
    Keyboard.press(slot1);//keyReport.keys[slot1] = key;
  }
  else if (!slot2) {
    slot2 = key;
    Keyboard.press(slot2);//keyReport.keys[slot2] = key;
  }
  else if (!slot3) {
    slot3 = key;
    Keyboard.press(slot3);//keyReport.keys[slot3] = key;
  }
  else if (!slot4) {
    slot4 = key;
    Keyboard.press(slot4);//keyReport.keys[slot4] = key;
  }
  else if (!slot5) {
    slot5 = key;
    Keyboard.press(slot5);//keyReport.keys[slot5] = key;
  }
  else if (!slot6) {
    slot6 = key;
    Keyboard.press(slot6);//keyReport.keys[slot6] = key;
  }
  if (!slot1 || !slot2 || !slot3 || !slot4 || !slot5 || !slot6)  {
    slots_full = LOW; // slots are not full
  }
  else {
    slots_full = HIGH; // slots are full
  } 
}
//
// Function to clear the slot that contains the key name
void clear_slot(int key) {
  if (slot1 == key) {
    slot1 = 0;
    Keyboard.release(key);
  }
  else if (slot2 == key) {
    slot2 = 0;
    Keyboard.release(key);
  }
  else if (slot3 == key) {
    slot3 = 0;
    Keyboard.release(key);
  }
  else if (slot4 == key) {
    slot4 = 0;
    Keyboard.release(key);
  }
  else if (slot5 == key) {
    slot5 = 0;
    Keyboard.release(key);
  }
  else if (slot6 == key) {
    slot6 = 0;
    Keyboard.release(key);
  }
  if (!slot1 || !slot2 || !slot3 || !slot4 || !slot5 || !slot6)  {
    slots_full = LOW; // slots are not full
  }
  else {
    slots_full = HIGH; // slots are full
  } 
}
//


void loop() {
  for (int x=0; x<rows_max; x++)  
  {
    go_0(Row_IO[x]);
    delayMicroseconds(10);

    for (int y = 0; y < cols_max; y++) 
    {
            if (modifier[x][y] != 0)     //Means there is modifier key present in that position
            {
                    if (!digitalRead(Col_IO[y]) && (old_key[x][y]))   //only executed key at the current position (x, y) is currently being pressed (the column is active), and it was not pressed in the previous iteration.avoid handling repeated key presses
                    {
                            if (modifier[x][y] != MODIFIERKEY_FN) 
                            {   // Exclude Fn modifier key  
                              load_mod(modifier[x][y]);
                              old_key[x][y] = LOW; // Save state of key as "pressed"
                            }

                            else 
                            {
                              Fn_pressed = LOW;
                              old_key[x][y] = LOW;
                            }
                    }

                    else if (digitalRead(Col_IO[y]) && (!old_key[x][y])) //responsible for handling normal keys (non-modifier keys) when they are released 
                    {  
                            if (modifier[x][y] != MODIFIERKEY_FN) 
                            { // Exclude Fn modifier key 
                              clear_mod(modifier[x][y]);
                              old_key[x][y] = HIGH; // Save state of key as "not pressed"
                            }

                            else 
                            {
                              Fn_pressed = HIGH; // Fn is no longer active
                              old_key[x][y] = HIGH; // old_key state is "not pressed" 
                            }
                    }
            }


            else if ((normal[x][y] != 0))     //Meaning there is a normal key at that position
            {
                    if (!digitalRead(Col_IO[y]) && (old_key[x][y]) && (!slots_full)) 
                    {
                      old_key[x][y] = LOW;

                            if (Fn_pressed) 
                            {  // Fn_pressed is active low so it is not pressed and normal key needs to be sent
                              load_slot(normal[x][y]);
                            }

                            else if(normal[x][y] == KEY_ESC)  
                            {

                              num_lock = !num_lock;

                                    if(num_lock)  
                                    {
                                      go_1(LED);
                                      normal[12][4] = 0;
                                      normal[5][4] = 0;
                                      normal[6][4] = 0;
                                      normal[7][4] = 0;
                                      normal[8][4] = 0;
                                      normal[8][6] = 0;
                                      normal[10][6] = 0;
                                      normal[10][4] = 0;
                                      normal[11][4] = 0;
                                      normal[13][4] = 0;
                                    }

                                    else  
                                    {
                                      go_0(LED);
                                      normal[12][4] = '0';
                                      normal[5][4] = '1';
                                      normal[6][4] = '2';
                                      normal[7][4] = '3';
                                      normal[8][4] = '4';
                                      normal[8][6] = '5';
                                      normal[10][6] = '6';
                                      normal[10][4] = '7';
                                      normal[11][4] = '8';
                                      normal[13][4] = '9';
                                    }
                            }

                            else if(normal[x][y] == KEY_CAPS_LOCK)
                            {
                               caps_lock = !caps_lock;
                               if(caps_lock)
                                 go_1(CAPS_LED);
                                else
                                  go_0(CAPS_LED);
                            }


                            else if(normal[x][y] == KEY_F7) 
                            {
                              digitalWrite(LCD, !digitalRead(LCD));
                            }

                            else if(normal[x][y] == KEY_F8) 
                            {
                              Keyboard.press(KEY_LEFT_GUI);
                              Keyboard.press('P');
                              delay(5);
                              Keyboard.release(KEY_LEFT_GUI);
                              Keyboard.release('P');
                            }

                            else if(normal[x][y] == KEY_F10) 
                            {
                              digitalWrite(KBDBKL, !digitalRead(KBDBKL));
                            }

                            else if(normal[x][y] == '7') 
                            {
                              digitalWrite(LTE, !digitalRead(LTE));
                            }
                            else if(normal[x][y] == KEY_PS) 
                            {
                              digitalWrite(WIFI, !digitalRead(WIFI));
                            }


                            else if (media[x][y] != 0) 
                            { // Fn is pressed so send media if a key exists in the matrix
                              Consumer.press(media[x][y]); // media key is sent using keyboard press function per PJRC    
                              delay(5); // delay 5 milliseconds before releasing to make sure it gets sent over USB
                              Consumer.release(); // send media key release
                              

                            }
                    }

                    else if (digitalRead(Col_IO[y]) && (!old_key[x][y])) 
                    {
                      old_key[x][y] = HIGH; // Save state of key as "not pressed"

                      if (Fn_pressed) 
                      {  // Fn is not pressed
                        clear_slot(normal[x][y]); //clear the slot that contains the normal key name
                      }
                    }        
            }
    }
    go_pu(Row_IO[x]);
  }
  delay(22); 
}


