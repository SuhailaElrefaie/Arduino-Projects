#include "funshield.h"
#include "input.h"

constexpr long interval = 1000;
constexpr long initialDelay = 1000;
constexpr long repeatedDelay = 300;
constexpr unsigned long baud_rate = 9600;

constexpr int digitCount = sizeof(digits) / sizeof(digits[0]);
constexpr int displayPositions = 4;
constexpr int minimumDigit = 0;
constexpr int maximumDigit = digitCount - 1;
constexpr unsigned long timeShift = 300;

constexpr int calculatePower(int base, int power) {
    return (power == 0) ? 1 : base * calculatePower(base, power - 1);
}

constexpr int dotPosition = 1;
constexpr unsigned long timeConversion = calculatePower(digitCount, dotPosition + 1);

constexpr byte emptyGlyphMask = 0b11111111;
constexpr byte dotGlyphMask = 0b01111111;
constexpr byte spaceGlyph = 0b00000000;
constexpr int allPositionsMask = (1 << displayPositions) - 1;

constexpr byte LETTER_GLYPH[] {
  0b10001000,   // A
  0b10000011,   // b
  0b11000110,   // C
  0b10100001,   // d
  0b10000110,   // E
  0b10001110,   // F
  0b10000010,   // G
  0b10001001,   // H
  0b11111001,   // I
  0b11100001,   // J
  0b10000101,   // K
  0b11000111,   // L
  0b11001000,   // M
  0b10101011,   // n
  0b10100011,   // o
  0b10001100,   // P
  0b10011000,   // q
  0b10101111,   // r
  0b10010010,   // S
  0b10000111,   // t
  0b11000001,   // U
  0b11100011,   // v
  0b10000001,   // W
  0b10110110,   // ksi
  0b10010001,   // Y
  0b10100100,   // Z
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

    void showChar(char symbol, int position) {
        byte glyph = emptyGlyphMask;
        
        if (isAlpha(symbol)) {
            if (isUpperCase(symbol))
                glyph = LETTER_GLYPH[symbol - 'A'];
            else {
                glyph = LETTER_GLYPH[symbol - 'a'];
            }
        } else if (isDigit(symbol)) {
            glyph = LETTER_GLYPH[symbol - '0'];
        } else if (isSpace(symbol)) {
            glyph = spaceGlyph;
        }

        if (position >= 0 && position < displayPositions) {
            byte positionMask = 1 << (displayPositions - 1 - position);
            showGlyph(glyph, positionMask);
        }
    }
};

Timer scrollTimer(timeShift);

class TextDisplay : public Display {
private:
    char text_[displayPositions];
    int currentIndex;
    bool isScrolling;
    const char* currentMessage;
    const char* messagePointer;
    Timer scrollStepTimer;
    int displayOffset;

public:
    void initialize() {
        Display::initialize();
        currentIndex = 0;
        isScrolling = false;
        currentMessage = nullptr;
        messagePointer = nullptr;
        scrollStepTimer.setInterval(timeShift);
        displayOffset = 0;
        for (int i = 0; i < displayPositions; ++i) {
            text_[i] = ' ';
        }
    }

    void startScrolling(const char* message, unsigned long currentTime_) {
        currentMessage = message;
        messagePointer = currentMessage;
        isScrolling = true;
        scrollStepTimer.resetTime(currentTime_);
        displayOffset = 0;
        for (int i = 0; i < displayPositions; ++i) {
            text_[i] = ' ';
        }
    }

    void updateScrolling() {
        if (!isScrolling) return;

        for (int i = 0; i < displayPositions; ++i) {
            const char* charPtr = currentMessage + displayOffset + i;
            if (*charPtr != '\0') {
                text_[i] = *charPtr;
            } else {
                text_[i] = ' ';
            }
        }
        displayOffset++;

        const char* temporaryPtr = currentMessage;
        int messageLength = 0;
        while (*temporaryPtr != '\0') {
            temporaryPtr++;
            messageLength++;
        }

        if (displayOffset > messageLength + displayPositions -1) {
            isScrolling = false;
            for (int i = 0; i < displayPositions; ++i) {
                text_[i] = ' ';
            }
        }
    }

    void update(unsigned long currentTime_) {
        if (isScrolling && scrollStepTimer.check(currentTime_)) {
            updateScrolling();
        }

        showChar(text_[currentIndex], currentIndex);
        currentIndex = (currentIndex + 1) % displayPositions;
    }

    bool isScrollingComplete() const {
        return !isScrolling;
    }
};

TextDisplay display;
SerialInputHandler inputHandler;

void setup() {
    display.initialize();
    inputHandler.initialize();
    Serial.begin(baud_rate);
}

void loop() {
    unsigned long currentTime = millis();
    inputHandler.updateInLoop();

    if (display.isScrollingComplete()) {
        const char* message = inputHandler.getMessage();
        display.startScrolling(message, currentTime);
    }

    display.update(currentTime);
}
