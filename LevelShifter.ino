#define IN0    2      /* D2 */ 
#define IN1    3      /* D3 */
#define IN2    4      /* D4 */
#define IN3    5      /* D5 */

#define OUT0   6      /* D6 */
#define OUT1   7      /* D7 */
#define OUT2   8      /* D8 */
#define OUT3   9      /* D9 */

#undef F
#define F(x) x

static const uint8_t g_Major = 0;
static const uint8_t g_Minor = 2;
static const char g_info[]   = F("info");
static const char g_low[]    = F("low");
static const char g_high[]   = F("high");
static const char g_cursor[] = F(">>");

static const uint32_t flashInterval  = 500; /* milliseconds */

struct pin_definition {
  const uint8_t pin;
  const char* name;
};

struct pin_definition input_pins[] =  { { IN0, F("IN0") }, { IN1, F("IN1") }, { IN2, F("IN2") }, { IN3, F("IN3") } };
struct pin_definition output_pins[] = { { OUT0, F("OUT0") }, { OUT1, F("OUT1") }, { OUT2, F("OUT2") }, { OUT3, F("OUT3") } };

void Help () {
  Serial.print(F("Levelshifter board"));
  Serial.print(g_Major);
  Serial.print(F("."));
  Serial.print(g_Minor);
  Serial.println(F("---------------------------------------"));
  Serial.println(F("'?' this screen"));
  Serial.println(F("'info' display the current settings of in and out"));
}

void Info() {

    // First time initialization, we report the current state of the output pins
  for (uint8_t index = 0; index < sizeof(output_pins)/sizeof(struct pin_definition); index++) {
    Serial.print(F("OUTPUT: "));
    Serial.print(output_pins[index].name);
    Serial.print(F("="));
    Serial.println(digitalRead(output_pins[index].pin) ? g_high : g_low);
  }

  // First time initialization, we report and record the current state for the input pins
  for (uint8_t index = 0; index < sizeof(input_pins)/sizeof(struct pin_definition); index++) {
    Serial.print("INPUT:  ");
    Serial.print(input_pins[index].name);
    Serial.print('=');

    Serial.println(digitalRead(input_pins[index].pin) == 0 ? g_low : g_high);
  } 
}

void UserEvaluation(const uint8_t length, const char buffer[]) {

  if ((length == 1) && (buffer[0] == '?')) {
    Help();
  } else if (strncmp (g_info, buffer, (sizeof(g_info)/sizeof(char) - 1)) == 0) {
    Info();
  }
  else {
    bool handled = false;
      
    for (uint8_t index = 0; ((handled == false) && (index < sizeof(output_pins)/sizeof(struct pin_definition))); index++) {
      const uint8_t keyLength = strlen(output_pins[index].name);
      if (strncmp (output_pins[index].name, buffer, keyLength) == 0) {
        handled = true;
        if ((buffer[keyLength] != '=') || (length != (keyLength + 2))) {
            Serial.println(F("Syntax should be '<pin name>=0' or '<pin name>=1'"));
        }
        else {
          const char value = toupper(buffer[keyLength + 1]);  
          if ((value == 'L') || (value == '0')) {
            digitalWrite(output_pins[index].pin, 0);
          }
          else if ((value == 'H') || (value == '1')) {
            digitalWrite(output_pins[index].pin, 1);
          }
          else {
            Serial.println(F("Unrecognized value, must be '0', '1', 'l', 'h', 'L' or 'H'"));
          }
        }
      }
    }

    if (handled == false) {
      Serial.print(F("Unrecognized verb: "));
      Serial.println(buffer);
    }
  }
  Serial.print(F(">>"));
}

void InputEvaluation(uint32_t& state) {
  // First time initialization, we report and record the current state
  for (uint8_t index = 0; index < sizeof(input_pins)/sizeof(struct pin_definition); index++) {
    bool value = !(digitalRead(input_pins[index].pin) == 0);

    // Check if we have a different state than we recorded..
    if (value ^ ((state & (1 << index)) != 0)) {
      // Yes, so time to notify
      Serial.println();
      Serial.print(F("INPUT:  "));
      Serial.print(input_pins[index].name);
      Serial.print(F("="));
      Serial.println(value ? g_high : g_low);
      Serial.print(F(">>"));

      // and record!!
      state ^= (1 << index);
    }
  }
}

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
static unsigned long g_previousFlashMillis = 0;

// This is the buffer to remember what the user is entering through the 
// serial console
static char    g_buffer[16];
static uint8_t g_length; 

static uint32_t g_lastState = 0;

void setup() {
  Serial.begin(115200);
  Serial.flush();

  // Initialize digital pin LED_BUILTIN as an output, just for DEBUG purpose.
  pinMode(LED_BUILTIN, OUTPUT);

  // First time initialization, we report the current state of the output pins
  for (uint8_t index = 0; index < sizeof(output_pins)/sizeof(struct pin_definition); index++) {
    pinMode(output_pins[index].pin, OUTPUT);
  }

  // First time initialization, we report and record the current state for the input pins
  for (uint8_t index = 0; index < sizeof(input_pins)/sizeof(struct pin_definition); index++) {
    pinMode(input_pins[index].pin, INPUT);
    if (digitalRead(input_pins[index].pin) != 0) {
      g_lastState |= (1 << index);
    }
  }
  Help();
  Info();
  Serial.print(g_cursor);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  if (static_cast<uint32_t>(currentMillis - g_previousFlashMillis) >= flashInterval) {
    // save the last time you blinked the LED
    g_previousFlashMillis = currentMillis;

    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

   while (Serial.available() > 0) {
    char character = Serial.read();
    if (::isprint(character)) {
      if (g_length < sizeof(g_buffer)) {
        g_buffer[g_length++] = character;    
        Serial.print(character);
      }
      else {
        Serial.print(F("*"));
      }
    }
    else {
      g_buffer[g_length] = '\0';
      Serial.println();
      UserEvaluation(g_length, g_buffer);
      g_length = 0;
      Serial.print(g_cursor);
    }
  }

  InputEvaluation(g_lastState);
}
