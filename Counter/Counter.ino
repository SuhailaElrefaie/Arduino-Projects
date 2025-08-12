#include "funshield.h"

constexpr int diodePins[] = { led1_pin, led2_pin, led3_pin, led4_pin };
constexpr int diodesCount = sizeof(diodePins) / sizeof(diodePins[0]);
constexpr int buttonPins[] = { button1_pin, button2_pin, button3_pin };
constexpr int buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);
constexpr long interval = 300;
constexpr long initialDelay = 1000;
constexpr long repeatedDelay = 300;
constexpr int maxCountValue = 1 << diodesCount;;
constexpr int incrementButton = 0;
constexpr int decrementButton = 1;

class Diode {
private:
    int diodeNumber_;
    bool currentState_;

public:
    void initialize(int diodeNumber) {
        diodeNumber_ = diodeNumber;
        pinMode(diodePins[diodeNumber], OUTPUT);
        change(false);
    }

    void change(bool newState) {
        digitalWrite(diodePins[diodeNumber_], newState ? LOW : HIGH);
        currentState_ = newState;
    }

    void change() {
        change(!currentState_);
    }
};

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

class Counter {
private:
    int value_;
    int maxCountValue_;
public:
    void initialize(int initialValue = 0, int maxCount = maxCountValue) {
        value_ = initialValue;
        maxCountValue_ = maxCount;
    }

    void increment() {
        value_ = (value_ + 1) % maxCountValue_;
    }

    void decrement() {
        value_ = (value_ - 1 + maxCountValue_) % maxCountValue_;
    }

    int getValue() {
        return value_;
    }
};

Diode diodes[diodesCount];
Button buttons[buttonCount];
Counter counter;

void displayCounter(int value) {
    for (int i = 0; i < diodesCount; ++i) {
        bool bitValue = ((value >> (diodesCount - 1 - i)) & 1) != 0;
        diodes[i].change(bitValue);
    }
}

void incrementCounter() {
    counter.increment();
}

void decrementCounter() {
    counter.decrement();
}

void setup() {
    for (int i = 0; i < diodesCount; ++i) {
        diodes[i].initialize(i);
    }
    for (int i = 0; i < buttonCount; ++i) {
        buttons[i].initialize(i);
    }
    counter.initialize();
    displayCounter(counter.getValue());
}

void loop() {
    unsigned long timeNow = millis();
    bool valueChange = false;
    for (int i = 0; i < buttonCount; ++i) {
        if (buttons[i].update(timeNow)) {
            if (incrementButton == i) {
                incrementCounter();
                valueChange = true; 
            }
            if (decrementButton == i) {
                decrementCounter();
                valueChange = true;
            }
        }
    }
    if (valueChange == true) {
        displayCounter(counter.getValue());
    }
}


