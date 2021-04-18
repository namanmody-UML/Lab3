#include <Wire.h>
#include <DS3231.h>
#include "IRremote.h"
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 17, 16, 15, 14);

// initialize the L293D module to control the DC motor
const uint8_t ENABLE    = 7; // Initializing PWM to pin 7 on the arduino  
const uint8_t DIRA      = 6; // Initializing IN1 to pin 6 on the arduino  
const uint8_t DIRB      = 5; // Initializing IN2 to pin 5 on the arduino

const uint8_t btn_pin   = 2; // Initializing the button to pin 2 on the arduino
const uint8_t receiver  = 3; // Initializing the IR receiver module to pin 3 on the arduino

boolean flagLCD       = false; // flag to 
boolean flagFAN       = false;
boolean flagDIR       = false;

IRrecv irrecv(receiver);        // create instance of 'irrecv'
decode_results results;         // create instance of 'decode_results'

volatile uint8_t stateA     = HIGH;
volatile uint8_t stateB     = LOW;
volatile uint8_t motorSpeed = 255;

DS3231 clock;
RTCDateTime dt;

void translateIR()
{
  switch(results.value)
  {
    case 33446055: fanDirectionChange(); break;
    case 33444015: motorSpeed = 50;     break;
    case 33478695: motorSpeed = 128;    break;
    case 33486855: motorSpeed = 180;    break;
    case 33435855: motorSpeed = 255;    break;
    default: 
    Serial.println(" other button   ");
  }
  delay(500);
}

void setup()
{
  pinMode(ENABLE,OUTPUT);
  pinMode(DIRA,OUTPUT);
  pinMode(DIRB,OUTPUT);
  Serial.begin(9600);

  Serial.println("Initialize RTC module");
  
  // Initialize DS3231
  clock.begin();
  
  // Send sketch compiling time to Arduino
  clock.setDateTime(__DATE__, __TIME__); 

  lcd.begin(16, 2); 

  // Disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Timer interrupt
  OCR1A = 0xF423;           // Set the value for 1s 
  TIMSK1 |= (1 << OCIE1A);  // Timer/Counter1 Output Compare Match A Interrupt Enable  
  sei();
  TCCR1B |= (1 << WGM12);   // clear timer on compare match (CTC)
  TCCR1B |= (1 << CS12);    // fclk/256 prescaler 

  // External button interrupt 
  pinMode(btn_pin, INPUT_PULLUP);
  EICRB |= (1 << ISC41);
  EIMSK |= (1 << INT4);

  // External IR receiver interrupt
  irrecv.enableIRIn();
  EICRB |= (1 << ISC50);
  EIMSK |= (1 << INT5);
}

void loop()
{
  if(flagLCD)
  {
    flagLCD = false;
    dt = clock.getDateTime();
    if(dt.second == 0)
      flagFAN = true;
    else if(dt.second == 30)
      flagFAN = false;
      
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(dt.hour); lcd.print(":");
    lcd.print(dt.minute); lcd.print(":");
    lcd.print(dt.second); lcd.print("");
    
    lcd.setCursor(0,1);
    lcd.print("rpm:"); lcd.print(motorSpeed); lcd.print(" ");
    
    if(flagDIR == false)
      lcd.print("forward"); 
    else
      lcd.print("backward"); 
  }
  fan();
}

void fan()
{
  dt = clock.getDateTime();
  if(flagFAN == true)
  {
    analogWrite(ENABLE, motorSpeed);
    digitalWrite(DIRA, stateA);
    digitalWrite(DIRB, stateB);
  }
  else 
    digitalWrite(ENABLE, LOW);
}

void fanDirectionChange()
{
  stateA = !stateA;
  stateB = !stateB;
  flagDIR = !flagDIR;
}

ISR(INT4_vect)
{
  fanDirectionChange();
}

ISR(INT5_vect)
{
  if (irrecv.decode(&results)) // have we received an IR signal?
  {
    translateIR(); 
    irrecv.resume(); // receive the next value
  } 
}

ISR(TIMER1_COMPA_vect)
{
  flagLCD = true;
}
