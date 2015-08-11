#define LEDPIN 2
#define ENABLEPIN 3

struct change_struct {
  unsigned long changems;
  unsigned char ledPin;
  unsigned char enablePin;
  unsigned char isr;
};

#define NCHANGES 32
struct change_struct changes[NCHANGES];
int serialChangePtr;       // Points ot next change to be printed to serial
volatile int isrChangePtr; // Points to next change to be used by ISR

#define PERIODIC_STATUS 60000
unsigned long lastStatusMs = 0;

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
    Serial.print((int) chg->ledPin);
    Serial.write('\t');
    Serial.print((int) chg->enablePin);
    Serial.write('\t');
    Serial.write(chg->isr);
    Serial.println();    
  }

  unsigned long now = millis();
  if (now > lastStatusMs + PERIODIC_STATUS) {
    int led = digitalRead(LEDPIN);
    int enable = digitalRead(ENABLEPIN);
    Serial.print(now);
    Serial.write('\t');
    Serial.print(led);
    Serial.write('\t');
    Serial.print(enable);
    Serial.write('\t');
    Serial.write('P');
    Serial.println();
    lastStatusMs = now;    
  }
}

void ledIsr(void) { addChange('L'); }
void enableIsr(void) { addChange('E'); }

void addChange(unsigned char isr)
{
  struct change_struct *chg = &(changes[isrChangePtr]);
  isrChangePtr++;
  if (isrChangePtr >= NCHANGES) {
    isrChangePtr = 0;
  }

  chg->changems = millis();
  chg->ledPin = digitalRead(LEDPIN);
  chg->enablePin = digitalRead(ENABLEPIN);
  chg->isr = isr;
}

