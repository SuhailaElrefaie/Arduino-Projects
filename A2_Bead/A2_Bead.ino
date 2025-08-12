// Assignment A2: Bead

#include "funshield.h"

constexpr int diodePins[] = { led1_pin, led2_pin, led3_pin, led4_pin };
constexpr int diodesCount = sizeof(diodePins) / sizeof(diodePins[0]);

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

Diode diodes[diodesCount];
int currentDiode;
bool ledFlowDirection;
constexpr long interval = 300;

class Timer {
private:
    unsigned long previousTime;
    unsigned long interval_;

public:
    Timer(unsigned long interval){
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
};

Timer diodeTimer(interval);

void moveBead() {
    diodes[currentDiode].change(false);

    if (ledFlowDirection == true) {
        if (currentDiode < diodesCount - 1) {
            currentDiode += 1;
        } else {
            ledFlowDirection = false;
            currentDiode -= 1;
        }
    } else {
        if (currentDiode > 0) {
            currentDiode -= 1;
        } else {
            ledFlowDirection = true;
            currentDiode += 1;
        }
    }

    diodes[currentDiode].change(true);
}

void setup() {
    for (int i = 0; i < diodesCount; ++i) {
        diodes[i].initialize(i);
    }
    currentDiode = 0;
    ledFlowDirection = true;
    diodes[currentDiode].change(true);
}

void loop() {
    unsigned long currentTime = millis();
    if (diodeTimer.check(currentTime) == true) {
        moveBead();
    }
}
