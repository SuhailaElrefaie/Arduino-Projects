#include "funshield.h"

constexpr int buttonPins[] = {button1_pin, button2_pin, button3_pin};
constexpr int buttonCount = sizeof(buttonPins) / sizeof(buttonPins[0]);

constexpr long interval = 1000;
constexpr long initialDelay = 1000;
constexpr long repeatedDelay = 300;
constexpr unsigned long baud_rate = 9600;

constexpr int incrementButton = 0;
constexpr int lapButton = 1;
constexpr int resetButton = 2;

constexpr int digitCount = sizeof(digits) / sizeof(digits[0]);
constexpr int displayPositions = 4;
constexpr int minimumDigit = 0;
constexpr int maximumDigit = digitCount - 1;

constexpr int calculatePower(int base, int power) {
    return (power == 0) ? 1 : base * calculatePower(base, power - 1);
}

constexpr int dotPosition = 1;
constexpr unsigned long timeConversion = calculatePower(digitCount, dotPosition + 1);

constexpr byte emptyGlyphMask = 0b11111111;
constexpr byte dotGlyphMask = 0b01111111;
constexpr int allPositionsMask = (1 << displayPositions) - 1;


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

        if ((currentState_ == true) && (previousState_ == false)) { 
            pressingButton_ = true;
            timer.setInterval(initialDelay);
            timer.resetTime(timeNow);
            triggered = true;
        } else if ((currentState_ == true) && (pressingButton_ == true)) { 
            if (timer.check(timeNow)) {
                triggered = true;
                timer.setInterval(repeatedDelay);
                timer.resetTime(timeNow);
            }
        } else if ((currentState_ == false) && (previousState_ == true)) { 
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

    void showGlyph(byte glyphMask, byte positionMask) {
        digitalWrite(latch_pin, LOW);
        shiftOut(data_pin, clock_pin, MSBFIRST, glyphMask);
        shiftOut(data_pin, clock_pin, MSBFIRST, positionMask);
        digitalWrite(latch_pin, HIGH);
    }

    void clear() {
        showGlyph(emptyGlyphMask, allPositionsMask);
    }

    void showDigit(int digit, int position, int minDigit = minimumDigit, int maxDigit = maximumDigit, bool dotActivated = false) {
        if (digit >= minDigit && digit <= maxDigit && position >= 0 && position < displayPositions) {
            byte glyph = digits[digit];
            if (dotActivated == true) {
                glyph &= dotGlyphMask;
            }
            byte positionMask = 1 << (displayPositions -1 - position);
            showGlyph(glyph, positionMask);
        }
    }
};

class NumericDisplay : public Display {
private:
    bool isActive_;
    int numberValue_;
    bool isDecimal_;
    int dotPosition_;
    int currentPosition_;
    int highestOrder_;
    int digitsToDisplay_[displayPositions];

    void calculateOrders() {
        highestOrder_ = 0;
        int currentNumber = numberValue_;
        
        for (int i = 0; i < displayPositions; ++i) {
            digitsToDisplay_[i] = currentNumber % digitCount;
            currentNumber /= digitCount;
            
            if (currentNumber > 0 && i >= highestOrder_)
                highestOrder_ = i + 1;
        }
        if (isDecimal_ && dotPosition_ > highestOrder_)
            highestOrder_ = dotPosition_;
    }

public:
    void initialize() {
        Display::initialize();
        currentPosition_ = 0;
        isActive_ = false;
        isDecimal_ = false;
        numberValue_ = 0;
        clear();
    }

    void setInteger(int newNumber) {
        numberValue_ = newNumber;
        isActive_ = true;
        isDecimal_ = false;
        calculateOrders();
    }

    void setDecimal(int newNumber, int dotPosition) {
        numberValue_ = newNumber;
        dotPosition_ = dotPosition;
        isActive_ = true;
        isDecimal_ = true;
        calculateOrders();
    }

    void deactivate() {
        isActive_ = false;
        clear();
    }

    void update() {
        if (!isActive_) return;

        if (currentPosition_ <= highestOrder_) {
            int digit = (numberValue_ / calculatePower(digitCount, currentPosition_)) % digitCount;
            bool dotOn = (isDecimal_ && currentPosition_ == dotPosition_);
            showDigit(digit, currentPosition_, minimumDigit, maximumDigit, dotOn);
        } else {
            clear();
        }

        currentPosition_ = (currentPosition_ + 1) % displayPositions;
    }
};

NumericDisplay display;

class Stopwatch {
public:
    enum class State {
        Stopped,
        Running,
        Lapped
    };

private:
    State currentState_;
    unsigned long startTime_;
    unsigned long lapTime_;
    unsigned long totalTime_;

public:
    Stopwatch() {
        currentState_ = State::Stopped;
        startTime_ = 0;
        lapTime_ = 0;
        totalTime_ = 0;
    }

    void startStop(unsigned long currentTime) {
        if (currentState_ == State::Stopped) {
            startTime_ = currentTime;
            currentState_ = State::Running;
        } else if (currentState_ == State::Running) {
            totalTime_ += currentTime - startTime_;
            currentState_ = State::Stopped;
        }
    }

    void lap(unsigned long currentTime) {
        if (currentState_ == State::Running) {
            lapTime_ = totalTime_ + (currentTime - startTime_);
            currentState_ = State::Lapped;
        } 
        else if (currentState_ == State::Lapped) {
            currentState_ = State::Running;
        }
    }

    void reset() {
        if (currentState_ == State::Stopped)
            totalTime_ = 0;
    }

    void update(unsigned long currentTime) {
        unsigned long elapsed = totalTime_;

        if (currentState_ == State::Stopped)
            elapsed = totalTime_;
        else if (currentState_ == State::Running)
            elapsed = totalTime_ + (currentTime - startTime_);
        else if (currentState_ == State::Lapped)
            elapsed = lapTime_;

        display.setDecimal(elapsed / timeConversion, dotPosition);
    }
};

Button buttons[buttonCount];
Stopwatch stopwatch;

void buttonActions(unsigned long currentTime) {
    if (buttons[incrementButton].update(currentTime))
        stopwatch.startStop(currentTime);
    
    if (buttons[lapButton].update(currentTime))
        stopwatch.lap(currentTime);
    
    if (buttons[resetButton].update(currentTime))
        stopwatch.reset();
}

void setup() {
    display.initialize();
    for (int i = 0; i < buttonCount; ++i) {
        buttons[i].initialize(i);
    }
    Serial.begin(baud_rate);
}

void loop() {
    unsigned long currentTime = millis();
    buttonActions(currentTime);
    stopwatch.update(currentTime);
    display.update();
}

