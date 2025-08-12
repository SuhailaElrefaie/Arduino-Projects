#include "funshield.h"

constexpr int buttonPins[] = { button1_pin, button2_pin, button3_pin };
constexpr int buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);

constexpr long interval = 1000;
constexpr long initialDelay = 1000;
constexpr long repeatedDelay = 300;
constexpr unsigned long baud_rate = 9600;

constexpr int incrementButton = 0;
constexpr int decrementButton = 1;
constexpr int orderButton = 2;

constexpr int digitCount = 10;
constexpr int displayPositions = 4;
constexpr int minimumDigit = 0;
constexpr int maximumDigit = 9;

constexpr byte emptyGlyphMask = 0b11111111;
constexpr int allPositionsMask = (1 << displayPositions) - 1;

constexpr int calculatePower(int base, int power) {
    return (power == 0) ? 1 : base * calculatePower(base, power - 1);
}

constexpr int maxDisplayValue = calculatePower(digitCount, displayPositions);


class Timer {
private:
    unsigned long previousTime;
    unsigned long interval_;

public:
    Timer() {
        previousTime = 0;
        interval_ = 0;
    }

    Timer(unsigned long interval) {
        previousTime = 0;
        interval_ = interval;
    }

    bool check(unsigned long currentTime) {
        if (currentTime - previousTime >= interval_) {
          previousTime += interval_;
          return true;
      }
      return false;
    }

    void resetTime(unsigned long currentTime) {
        previousTime = currentTime;
    }

    void setInterval(unsigned long newInterval) {
        interval_ = newInterval;
    }
};

class Button {
private:
    int buttonNumber_;
    bool currentState_;
    bool previousState_;
    bool pressingButton_;
    Timer timer;

public:
    Button() {
        timer = Timer(initialDelay);
        currentState_ = false;
        previousState_ = false;
        pressingButton_ = false;
    }

    void initialize(int buttonNumber) {
        buttonNumber_ = buttonNumber;
        pinMode(buttonPins[buttonNumber], INPUT);
    }

    bool pressed() {
        return digitalRead(buttonPins[buttonNumber_]) == LOW;
    }

    bool update(unsigned long timeNow) {
        currentState_ = pressed();
        bool triggered = false;

        if ((currentState_ == true) && (previousState_ == false)) { // Initial press of button
            pressingButton_ = true;
            timer.setInterval(initialDelay);
            timer.resetTime(timeNow);
            triggered = true;
    
        } else if ((currentState_ == true) && (pressingButton_ == true)) { // Button still held down after initial press
            if (timer.check(timeNow)) {
                triggered = true;
                timer.setInterval(repeatedDelay);
                timer.resetTime(timeNow);
            }
          } else if ((currentState_ == false) && (previousState_ == true)) { // Button released
              pressingButton_ = false;
          }

        previousState_ = currentState_;
        return triggered;
    }
};

class Display {
public:
    void initialize() {
        pinMode(latch_pin, OUTPUT);
        pinMode(data_pin, OUTPUT);
        pinMode(clock_pin, OUTPUT);
        clear();
    }
    void showGlyph (byte glyphMask, byte positionMask) {
        digitalWrite(latch_pin, LOW);
        shiftOut(data_pin, clock_pin, MSBFIRST, glyphMask);
        shiftOut(data_pin, clock_pin, MSBFIRST, positionMask);
        digitalWrite(latch_pin, HIGH);
    }

    void clear() {
        showGlyph(emptyGlyphMask, allPositionsMask);
    }

    void showDigit(int digit, int position, int minDigit = minimumDigit, int maxDigit = maximumDigit) {
        if (digit >= minDigit && digit <= maxDigit && position >= 0 && position < displayPositions) {
            byte glyph = digits[digit];
            byte positionMask = 1 << (displayPositions -1 - position);
            showGlyph(glyph, positionMask);
        }
    }
};

Button buttons[buttonCount];
Display display;

class Counter {
private:
    int currentValue;
    int currentPosition;
    int maxCounterValue;

public:
    void initialize(int startValue = 0, int startPosition = 0, int maxValue = maxDisplayValue) {
        currentValue = startValue;
        currentPosition = startPosition;
        maxCounterValue = maxValue;
    }

    void increment(int currentPower) {
        currentValue = (currentValue + currentPower) % maxCounterValue;
    }

    void decrement(int currentPower) {
        currentValue = (currentValue - currentPower + maxCounterValue) % maxCounterValue;
    }

    void changeOrder() {
        currentPosition = (currentPosition + 1) % displayPositions;
    }

    void updateDisplay(int currentPower) {
        int digit = (currentValue / currentPower) % digitCount;
        display.showDigit(digit, currentPosition);
    }

    void update(unsigned long currentTime) {
        int currentPower = calculatePower(digitCount, currentPosition);

        if (buttons[incrementButton].update(currentTime))
            increment(currentPower);
        
        if (buttons[decrementButton].update(currentTime))
            decrement(currentPower);
        
        if (buttons[orderButton].update(currentTime))
            changeOrder();
    
        updateDisplay(currentPower);
        
    }
};

Counter counter;

void setup() {
    display.initialize();
    for (int i = 0; i < buttonCount; ++i) {
        buttons[i].initialize(i);
    }
    counter.initialize(); 
}

void loop() {
    unsigned long currentTime = millis();
    counter.update(currentTime);
}

