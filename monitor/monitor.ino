#define LEDPIN 2
#define ENABLEPIN 3

struct change_struct {
  unsigned long changems;
  unsigned long changeus;
  unsigned char ledPin;
  unsigned char enablePin;
  unsigned char isLedIsr;
  unsigned char isEnableIsr;
};

#define NCHANGES 32
struct change_struct changes[NCHANGES];
int serialChangePtr;       // Points ot next change to be printed to serial
volatile int isrChangePtr; // Points to next change to be used by ISR

void setup() {
  pinMode(LEDPIN, INPUT);
  isrChangePtr = 0;
  serialChangePtr = 0;

  attachInterrupt(0, ledIsr, CHANGE);
  attachInterrupt(1, enableIsr, CHANGE);

  Serial.begin(9600);
}

void loop() {
  if (isrChangePtr != serialChangePtr) {
    struct change_struct *chg = &(changes[serialChangePtr]);
    serialChangePtr++;
    if (serialChangePtr >= NCHANGES) {
      serialChangePtr = 0;
    }

    Serial.print(chg->changems);
    Serial.write('\t');
    Serial.print(chg->changeus);
    Serial.write('\t');
    Serial.print((int) chg->ledPin);
    Serial.write('\t');
    Serial.print((int) chg->enablePin);
    Serial.println();    
  }
}

void ledIsr(void) { addChange(1, 0); }
void enableIsr(void) { addChange(0, 1); }

void addChange(int isLed, int isEnable)
{
  struct change_struct *chg = &(changes[isrChangePtr]);
  isrChangePtr++;
  if (isrChangePtr >= NCHANGES) {
    isrChangePtr = 0;
  }

  chg->changems = millis();
  chg->changeus = micros();
  chg->ledPin = digitalRead(LEDPIN);
  chg->enablePin = digitalRead(ENABLEPIN);
  chg->isLedIsr = isLed;
  chg->isEnableIsr = isEnable;
}

