#define LEDPIN 2
#define ENABLEPIN 3
// On Arduino Uno, pin #2 is PD2 and pin #3 is PD3
// Port D input register is PIND, bit masks are 0x04 and 0x08
volatile uint8_t *pinsRegister;
uint8_t ledPinMask;
uint8_t enablePinMask;

struct change_struct {
  unsigned long changems;
  uint8_t pind;
  unsigned char isr;
};

#define NCHANGES 32
struct change_struct changes[NCHANGES];
uint8_t serialChangePtr;       // Points ot next change to be printed to serial
volatile uint8_t isrChangePtr; // Points to next change to be used by ISR

#define PERIODIC_STATUS 60000
unsigned long lastStatusMs = 0;

void setup() {
  pinMode(LEDPIN, INPUT);
  isrChangePtr = 0;
  serialChangePtr = 0;

  pinsRegister = portInputRegister(digitalPinToPort(LEDPIN));  
  ledPinMask = digitalPinToBitMask(LEDPIN);
  enablePinMask = digitalPinToBitMask(ENABLEPIN);

  attachInterrupt(0, ledIsr, CHANGE);
  attachInterrupt(1, enableIsr, CHANGE);

  Serial.begin(9600);

  if (pinsRegister != portInputRegister(digitalPinToPort(ENABLEPIN))) {
    Serial.println("!!! LED and enable pins on different ports");
    enablePinMask = 0;
  }
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
    Serial.print((chg->pind & ledPinMask) ? '1' : '0');
    Serial.write('\t');
    Serial.print((chg->pind & enablePinMask) ? '1' : '0');
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

// Avoid function call overhead to the extent possible in ISRs
// N.B. millis() relies on a private volatile counter in wiring
void ledIsr(void) 
{ 
  struct change_struct *chg = &(changes[isrChangePtr]);
  isrChangePtr = (isrChangePtr + 1) % NCHANGES;

  chg->changems = millis();
  chg->pind = *pinsRegister;
  chg->isr = 'L';
}
  
void enableIsr(void)
{
  struct change_struct *chg = &(changes[isrChangePtr]);
  isrChangePtr = (isrChangePtr + 1) % NCHANGES;

  chg->changems = millis();
  chg->pind = *pinsRegister;
  chg->isr = 'E';
}


