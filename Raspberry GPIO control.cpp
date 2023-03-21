// Raspberry GPIO control.cpp : Defines the entry point for the application.
//

#include "Raspberry GPIO control.h"
using namespace std;

const string enablePort = "18";
const string a1Port = "23";
const string a2Port = "24";
const string b1Port = "5";
const string b2Port = "6";


class Port {
private:
	string portNumber;
	bool isOn;
	bool out;
	bool high;

public:
	Port(string portNumber, bool out, bool high) {
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
		isOn = true;
		string command = "gpio mode " + portNumber + " " + (out ? "out" : "in");
		cout << "DEBUG: " << command << endl;
		system(command.c_str());
	}

	void ToggleHigh() {
		SetHigh(!high);
	}

	void SetHigh(bool high) {
		this->high = high;
		string command = "gpio write " + portNumber + " " + (high ? "1" : "0");
		cout << "DEBUG: " << command << endl;
		system(command.c_str());
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
	WheelControl(string enablePort, HBridgePair* RightWheel, HBridgePair* LeftWheel) {
		enable = new Port(enablePort, true, true);
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
		enable->SetHigh(false);
	}

};


int main()
{
	WheelControl* wheelControl;
	cout << "Starting the Engine" << endl;
	Port* A1 = new Port(a1Port, true, false);
	Port* A2 = new Port(a2Port, true, false);
	HBridgePair* A = new HBridgePair(A1, A2);

	Port* B1 = new Port(b1Port, true, false);
	Port* B2 = new Port(b2Port, true, false);
	HBridgePair* B = new HBridgePair(B1, B2);

	wheelControl = new WheelControl(enablePort, A, B);
	wheelControl->Initialize();
	return 0;
}

void GetInput(WheelControl* wheelControl) {
	cout << "Input Command:";
	string command;
	cin >> command;
	
	if (command == "Forward") {
		wheelControl->Forward();
		return;
	}
	if (command == "Reverse") {
		wheelControl->Reverse();
		return;
	}
	if (command == "TurnLeft") {
		wheelControl->TurnLeft();
		return;
	}
	if (command == "TurnRight") {
		wheelControl->TurnRight();
		return;
	}
	if (command == "Stop") {
		wheelControl->Stop();
		return;
	}
	if (command == "q") {
		throw;
	}

	cout << "Unrecognized. Firetruck." << endl;
}
