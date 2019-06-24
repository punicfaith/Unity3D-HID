#include "TrinketHidCombo.h"

#define PIN_ENCODER_A 0
#define PIN_ENCODER_B 2
#define PIN_ENCODER_SWITCH 1
#define SEPARATOR_LEN 3

enum click_type{
  None,
  Pressed,
  Click,
  DoubleClick,
  LongPressed,
  LongPressed2,
};

static uint8_t enc_prev_pos = 0;
static uint8_t enc_flags    = 0;
static int8_t  enc_action   = 0;
static bool isDoubleClickBegin = false;
static bool toggleMode = false;

click_type btnClickType = None;

void setup() {
  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  digitalWrite(PIN_ENCODER_A, HIGH);
  digitalWrite(PIN_ENCODER_B, HIGH);

  pinMode(PIN_ENCODER_SWITCH, INPUT);

  TrinketHidCombo.begin();
  
  if (digitalRead(PIN_ENCODER_A) == LOW) {
    enc_prev_pos |= (1 << 0);
  }
  if (digitalRead(PIN_ENCODER_B) == LOW) {
    enc_prev_pos |= (1 << 1);
  }
}


void loop() {
  read_direction();
  click_event();

  if (is_right() || is_left()) {
    if(is_right()){
      volum_control(MMKEY_VOL_UP);
    } else {
      volum_control(MMKEY_VOL_DOWN);
    }
  } 
  
  if (btnClickType != None) {
    switch(btnClickType){
      case Pressed :
        if(toggleMode){
          TrinketHidCombo.pressKey(KEYCODE_MOD_LEFT_ALT | KEYCODE_MOD_LEFT_CONTROL, KEYCODE_P);
          TrinketHidCombo.pressKey(0, 0); // 버튼 클릭 해제
        }
        break;
      case Click :
        if(toggleMode == false){
          TrinketHidCombo.pressMultimediaKey(MMKEY_SCAN_NEXT_TRACK);
        }
        break;
      case DoubleClick :
        //TrinketHidCombo.pressMultimediaKey(MMKEY_SCAN_NEXT_TRACK);
        toggleMode = !toggleMode;
        break;
      case LongPressed :
        
        break;
      case LongPressed2 :
        
        break;
      default :
        break;
    }
    
    delay(100);
  }

  TrinketHidCombo.poll();
}


void volum_control(int key) {
  for(int i = 0; i < SEPARATOR_LEN; ++i){
    TrinketHidCombo.pressMultimediaKey(key);
  }
}



// 이하 코드 다른 곳에서 복붙
void read_direction() {
  enc_action = 0; // reset
  uint8_t enc_cur_pos = 0;

  // read in the encoder state first
  if (!digitalRead(PIN_ENCODER_A)) {
    enc_cur_pos |= (1 << 0);
  }
  if (!digitalRead(PIN_ENCODER_B)) {
    enc_cur_pos |= (1 << 1);
  }

  // if any rotation at all
  if (enc_cur_pos != enc_prev_pos) {
    if (enc_prev_pos == 0x00) {
      // this is the first edge
      if (enc_cur_pos == 0x01) {
        enc_flags |= (1 << 0);
      } else if (enc_cur_pos == 0x02) {
        enc_flags |= (1 << 1);
      }
    }

    if (enc_cur_pos == 0x03) {
      // this is when the encoder is in the middle of a "step"
      enc_flags |= (1 << 4);
    } else if (enc_cur_pos == 0x00) {
      // this is the final edge
      if (enc_prev_pos == 0x02) {
        enc_flags |= (1 << 2);
      }
      else if (enc_prev_pos == 0x01) {
        enc_flags |= (1 << 3);
      }
 
      if (bit_is_set(enc_flags, 0) && (bit_is_set(enc_flags, 2) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      } else if (bit_is_set(enc_flags, 2) && (bit_is_set(enc_flags, 0) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      } else if (bit_is_set(enc_flags, 1) && (bit_is_set(enc_flags, 3) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      } else if (bit_is_set(enc_flags, 3) && (bit_is_set(enc_flags, 1) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      }
      enc_flags = 0; // reset for next time
    }
  }
 
  enc_prev_pos = enc_cur_pos;
}

bool is_right() {
  return enc_action > 0;
}

bool is_left() {
  return enc_action < 0;
}

int debounce = 20;          // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 250;            // max ms between clicks for a double click event
int holdTime = 1000;        // ms hold period: how long to wait for press+hold event
int longHoldTime = 3000;    // ms long hold period: how long to wait for press+hold event
boolean buttonVal = HIGH;   // value read from button
boolean buttonLast = HIGH;  // buffered value of the button's previous state
boolean DCwaiting = false;  // whether we're waiting for a double click (down)
boolean DConUp = false;     // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true;    // whether it's OK to do a single click
long downTime = -1;         // time the button was pressed down
long upTime = -1;           // time the button was released
boolean ignoreUp = false;   // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false;        // when held, whether to wait for the up event
boolean holdEventPast = false;    // whether or not the hold event happened already
boolean longHoldEventPast = false;// whether or not the long hold event happened already


void click_event() {
  btnClickType = None;
  int clickCheckCount = 80;

  buttonVal = !digitalRead(PIN_ENCODER_SWITCH);

  // Button pressed down
   if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce)
   {
      btnClickType = Pressed;
      downTime = millis();
      ignoreUp = false;
      waitForUp = false;
      singleOK = true;
      holdEventPast = false;
      longHoldEventPast = false;
      if ((millis()-upTime) < DCgap && DConUp == false && DCwaiting == true)  DConUp = true;
      else  DConUp = false;
      DCwaiting = false;
   }
   // Button released
   else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce)
   {        
       if (not ignoreUp)
       {
           upTime = millis();
           if (DConUp == false) {
            DCwaiting = true;
            } else {
               btnClickType = DoubleClick;
               DConUp = false;
               DCwaiting = false;
               singleOK = false;
           }
       }
   }
   // Test for normal click event: DCgap expired
   if ( buttonVal == HIGH && (millis()-upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true && btnClickType != DoubleClick)
   {
       btnClickType = Click;
       DCwaiting = false;
   }
   // Test for hold
   if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
       // Trigger "normal" hold
       if (not holdEventPast)
       {
           btnClickType = LongPressed;
           waitForUp = true;
           ignoreUp = true;
           DConUp = false;
           DCwaiting = false;
           //downTime = millis();
           holdEventPast = true;
       }
       // Trigger "long" hold
       if ((millis() - downTime) >= longHoldTime)
       {
           if (not longHoldEventPast)
           {
               btnClickType = LongPressed2;
               longHoldEventPast = true;
           }
       }
   }
   
   buttonLast = buttonVal;
}
