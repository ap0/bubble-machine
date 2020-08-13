#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int STEPS_PER_REVOLUTION = 2038;

const int WAND_POT_PIN = 1; //A1;
const int FAN_POT_PIN = 0;  //A0;

const int FAN_MOSFET_PIN = 3;

const int MAX_WAND_SPEED = 13;
const int MAX_FAN_SPEED = 21;

const int BUFFER_SIZE = 8;
const int REFRESH_INTERVAL = 500; //ms

const int LCD_WIDTH = 16;
const int LCD_HEIGHT = 2;

// We use rolling buffers of fixed size to track the last N values of a pin
// and then average them out to determine whether or not to change the value.
int wandVals[BUFFER_SIZE];
int fanVals[BUFFER_SIZE];
int wandPos = 0;
int fanPos = 0;

// Track fan speeds to make sure we're not updating unnecessarily
int lastWandSpeed = -1;
int lastFanSpeed = -1;

bool screenDirty = false;

const char *intro[] = {
    "oO0* Nora's *oO0",
    "*Oo Bubbles! oO*",
};
char lcdBuffer[17];

unsigned long lastUpdate = 0;
unsigned long lastStepMicros = 0;

/*
Note that I have modified how the stepper control works.  I'm not using the library but instead
ripped out the 4-pin version of it and have it process in the main event loop.

If you don't do this, you will have issues where the LCD and motor block each other and the whole system
appears very jittery.  This makes everything much smoother.
*/

// ** This is not the same wiring as you find on most examples!  This is so the circuit board wiring is simpler (see the schematic) **
// These values are just mapped from the standard 8, 10, 9, 11 configuration.
const int STEP_PIN_1 = 11;
const int STEP_PIN_2 = 9;
const int STEP_PIN_3 = 10;
const int STEP_PIN_4 = 8;

unsigned long stepDelay = 0;
int curStep = 0;

// Convert RPM to microseconds per step for use in the event loop.
void setStepperRPM(long rpm)
{
    if (rpm == 0)
    {
        stepDelay = 0;
        return;
    }
    stepDelay = 60L * 1000L * 1000L / STEPS_PER_REVOLUTION / rpm;
}

// Copied straight from Stepper.h
void stepMotor(int stepNum)
{
    switch (stepNum)
    {
    case 0: // 1010
        digitalWrite(STEP_PIN_1, HIGH);
        digitalWrite(STEP_PIN_2, LOW);
        digitalWrite(STEP_PIN_3, HIGH);
        digitalWrite(STEP_PIN_4, LOW);
        break;
    case 1: // 0110
        digitalWrite(STEP_PIN_1, LOW);
        digitalWrite(STEP_PIN_2, HIGH);
        digitalWrite(STEP_PIN_3, HIGH);
        digitalWrite(STEP_PIN_4, LOW);
        break;
    case 2: //0101
        digitalWrite(STEP_PIN_1, LOW);
        digitalWrite(STEP_PIN_2, HIGH);
        digitalWrite(STEP_PIN_3, LOW);
        digitalWrite(STEP_PIN_4, HIGH);
        break;
    case 3: //1001
        digitalWrite(STEP_PIN_1, HIGH);
        digitalWrite(STEP_PIN_2, LOW);
        digitalWrite(STEP_PIN_3, LOW);
        digitalWrite(STEP_PIN_4, HIGH);
        break;
    }
}

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

const char randChars[] = "o0O *";
char bufStr[LCD_WIDTH + 1];

// Prints random 'bubble' characters in the specified portions of the LCD
void randLcd(int start, int len, int line)
{
    for (int i = 0; i < len; i++)
    {
        bufStr[i] = randChars[random(5)];
    }
    bufStr[len] = '\0';

    lcd.setCursor(start, line);
    lcd.print(bufStr);
}

void setup()
{
    randomSeed(analogRead(2));
    pinMode(FAN_MOSFET_PIN, OUTPUT);
    pinMode(STEP_PIN_1, OUTPUT);
    pinMode(STEP_PIN_2, OUTPUT);
    pinMode(STEP_PIN_3, OUTPUT);
    pinMode(STEP_PIN_4, OUTPUT);

    initArray(wandVals, BUFFER_SIZE);
    initArray(fanVals, BUFFER_SIZE);

    Wire.begin();
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print(intro[0]);
    lcd.setCursor(0, 1);
    lcd.print(intro[1]);

    // Sample inputs before we get started
    int startTime = millis();

    while (millis() - startTime < 2000)
    {

        int wandVal = analogRead(WAND_POT_PIN);
        int fanVal = analogRead(FAN_POT_PIN);
        updateArray(wandVal, wandVals, &wandPos, BUFFER_SIZE);
        updateArray(fanVal, fanVals, &fanPos, BUFFER_SIZE);

        randLcd(0, 4, 0);
        randLcd(12, 4, 0);
        randLcd(0, 3, 1);
        randLcd(13, 3, 1);
        delay(150);
    }

    // Set up template values -- we will only refresh the number elements in the loop.
    lcd.setCursor(0, 0);

    //  lcd.print("Fan Speed:    %");
    lcd.print("oO0oOo Fan:    %");
    lcd.setCursor(0, 1);
    //  lcd.print("Wand Speed:    %");
    lcd.print("oO0Oo Wand:    %");
}

int average(int vals[], int arrSize)
{
    int sum = 0;
    for (int i = 0; i < arrSize; i++)
    {
        sum += vals[i];
    }

    return sum / arrSize;
}

int initArray(int vals[], int arrSize)
{
    for (int i = 0; i < arrSize; i++)
    {
        vals[i] = 0;
    }
}

// Adds a value to the buffer and returns the average value of it.
int updateArray(int val, int vals[], int *pos, int arrSize)
{
    // Only dereference once
    int thisPos = *pos;
    if (thisPos < 0 || thisPos >= arrSize)
        *pos = 0;

    vals[thisPos] = val;

    if (thisPos == arrSize - 1)
    {
        *pos = 0;
    }
    else
    {
        *pos = thisPos + 1;
    }

    return average(vals, arrSize);
}

bool firstLineUpdate = false;
int firstLineCursor = 0;
int secondLineCursor = 0;
void loop()
{

    int wandVal = updateArray(analogRead(WAND_POT_PIN), wandVals, &wandPos, BUFFER_SIZE);
    int fanVal = updateArray(analogRead(FAN_POT_PIN), fanVals, &fanPos, BUFFER_SIZE);

    // Use unsigned ints so they can never go below 0.  I saw the value be -8 once and was baffled.
    unsigned long wandSpeed = map(wandVal, 0, 1023, 0, MAX_WAND_SPEED);
    unsigned long fanSpeed = map(fanVal, 0, 1023, 0, MAX_FAN_SPEED);

    // HACK
    // Do -1 because it sometimes flops at the last increment between MAX and MAX-1
    // Here we just make the actual max MAX-1.
    wandSpeed = (wandSpeed >= MAX_WAND_SPEED ? MAX_WAND_SPEED - 1 : wandSpeed);
    if (wandSpeed != lastWandSpeed)
    {
        // wandStepper.setSpeed(wandSpeed);
        setStepperRPM(wandSpeed);

        lastWandSpeed = wandSpeed;

        screenDirty = true;
    }

    // Use the current time of the loop to determine when to move to the next step.
    // This smooths out the motion while also letting the LCD update.
    // It might not be perfectly tight from a precision perspective but it's close enough.

    fanSpeed = (fanSpeed >= MAX_FAN_SPEED ? MAX_FAN_SPEED - 1 : fanSpeed);
    if (fanSpeed != lastFanSpeed)
    {
        // Don't go full 255 here.
        // Motor already draws enough current as it is.
        analogWrite(FAN_MOSFET_PIN, map(fanSpeed, 0, MAX_FAN_SPEED, 0, 245));

        lastFanSpeed = fanSpeed;

        screenDirty = true;
    }

    // Loop until we can step the motor.
    // After we step it, update the screen if necessary.
    while (true)
    {
        unsigned long now = micros();
        // We want to make sure we don't get into an infinite loop if the stepper isn't moving.

        if (now - lastStepMicros >= stepDelay || stepDelay == 0)
        {
            if (stepDelay > 0)
            {
                stepMotor(curStep % 4);
                curStep++;
                if (curStep == 4)
                {
                    curStep = 0;
                }
            }
            lastStepMicros = now;

            // Only do screen updates immediately after stepping the motor.
            if (screenDirty)
            {
                // Only update the values we need to
                sprintf(lcdBuffer, "%3d", map(fanSpeed, 0, MAX_FAN_SPEED - 1, 0, 100));
                lcd.setCursor(12, 0);
                lcd.print(lcdBuffer);

                sprintf(lcdBuffer, "%3d", map(wandSpeed, 0, MAX_WAND_SPEED - 1, 0, 100));
                lcd.setCursor(12, 1);
                lcd.print(lcdBuffer);

                screenDirty = false;
            }

            if (now - lastUpdate > REFRESH_INTERVAL * 1000L)
            {
                if (firstLineUpdate)
                {
                    randLcd(0, 6, 0);
                }
                else
                {
                    randLcd(0, 5, 1);
                }
                firstLineUpdate = !firstLineUpdate;
                lastUpdate = now;
            }
            return;
        }
    }
}
