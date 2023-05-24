#include <iostream>
#include <fstream>
#include <signal.h>

#include <wiringPi.h>          // to control Raspberry Pi digital pins
#include "src/datetime.h"      // to have current time for logging

// Raspberry PI prerequisites:
// install WiringPi

using namespace jed_utils;     // for datetime
using namespace std;

#define	enablePin	1
#define	left1Pin	4
#define	left2Pin	5
#define	right1Pin	21
#define	right2Pin	22

bool inputTextPrinted = false;

class Port {
private:
	int portNumber;
	bool isOn;
	bool out;
	bool high;

public:
	Port(int portNumber, bool out, bool high) {
		this->portNumber = portNumber;
		this->out = out;
		this->high = high;
		isOn = false;
	}

	bool IsOn() {
		return isOn;
	}

	bool IsHigh() {
		return high;
	}

	void TurnOn() {
		pinMode(portNumber, OUTPUT);

		isOn = true;
		cout << "DEBUG: " << "gpio mode " << portNumber << " " << (out ? "out" : "in") << endl;
	}

	void ToggleHigh() {
		SetHigh(!high);
	}

	void SetHigh(bool high) {
		if (high) {
			digitalWrite(portNumber, HIGH);
		}
		else {
			digitalWrite(portNumber, LOW);
		}

		this->high = high;

		cout << "DEBUG: " << "gpio write " << portNumber << " " << (high ? "1" : "0") << endl;
	}
};

class HBridgePair {
private:
	Port* one;
	Port* two;

public:
	HBridgePair(Port* one, Port* two) {
		this->one = one;
		this->two = two;
	}

	void Initialize() {
		one->TurnOn();
		two->TurnOn();
	}

	void Stop() {
		one->SetHigh(false);
		two->SetHigh(false);
	}

	void Forward() {
		one->SetHigh(true);
		two->SetHigh(false);
	}

	void Reverse() {
		one->SetHigh(false);
		two->SetHigh(true);
	}

	void ChangeDirection() {
		if (one->IsHigh() == two->IsHigh()) return; //changing direction means nothing if you dont move

		one->ToggleHigh();
		two->ToggleHigh();
	}
};

class WheelControl {
private:
	Port* enable;
	HBridgePair* RightWheel;
	HBridgePair* LeftWheel;

public:
	WheelControl(int EnablePort, HBridgePair* LeftWheel, HBridgePair* RightWheel) {
		enable = new Port(EnablePort, true, true);
		this->RightWheel = RightWheel;
		this->LeftWheel = LeftWheel;
	}

	void Initialize() {
		enable->TurnOn();
		RightWheel->Initialize();
		LeftWheel->Initialize();
	}

	void TurnOn() {
		enable->SetHigh(true);
	}

	void Forward() {
		RightWheel->Forward();
		LeftWheel->Forward();
		TurnOn();
	}

	void Reverse() {
		RightWheel->Reverse();
		LeftWheel->Reverse();
		TurnOn();
	}

	void TurnLeft() {
		RightWheel->Forward();
		LeftWheel->Reverse();
		TurnOn();
	}

	void TurnRight() {
		RightWheel->Reverse();
		LeftWheel->Forward();
		TurnOn();
	}

	void Stop() {
		RightWheel->Stop();
		LeftWheel->Stop();
		enable->SetHigh(false);
	}

};

void GetInput(WheelControl* wheelControl);

bool ctrl_c_pressed;
void ctrlc(int) {
	ctrl_c_pressed = true;
}

int main(void)
{
	std::cout << jed_utils::datetime().to_string() << " App started\n";

	// set-up movement controls
	wiringPiSetup();

	std::cout << jed_utils::datetime().to_string() << " Setting up wheel controls" << endl;

	WheelControl* wheelControl;
	Port* A1 = new Port(left1Pin, true, false);
	Port* A2 = new Port(left2Pin, true, false);
	HBridgePair* A = new HBridgePair(A1, A2);

	Port* B1 = new Port(right1Pin, true, false);
	Port* B2 = new Port(right2Pin, true, false);
	HBridgePair* B = new HBridgePair(B1, B2);

	wheelControl = new WheelControl(enablePin, A, B);
	wheelControl->Initialize();

	// create detection of ctrl + c pressed
	signal(SIGINT, ctrlc);

	while (1) {
		GetInput(wheelControl);
		if (ctrl_c_pressed) {
			break;
		}
	}

	// end program
	std::cout << jed_utils::datetime().to_string() << " App cancellation initiated\n";

	return 0;
}

void GetInput(WheelControl* wheelControl) {
	if (!inputTextPrinted) {
		inputTextPrinted = true;
		std::cout << jed_utils::datetime().to_string() << " Input Command:";
	}

	string command;
	std::cin >> command;

	if (command == "Forward") {
		inputTextPrinted = false;
		wheelControl->Forward();
		return;
	}

	if (command == "Reverse") {
		inputTextPrinted = false;
		wheelControl->Reverse();
		return;
	}

	if (command == "TurnLeft") {
		inputTextPrinted = false;
		wheelControl->TurnLeft();
		return;
	}

	if (command == "TurnRight") {
		inputTextPrinted = false;
		wheelControl->TurnRight();
		return;
	}

	if (command == "Stop") {
		inputTextPrinted = false;
		wheelControl->Stop();
		return;
	}

	cout << jed_utils::datetime().to_string() << " Unrecognized. Firetruck\n";
}