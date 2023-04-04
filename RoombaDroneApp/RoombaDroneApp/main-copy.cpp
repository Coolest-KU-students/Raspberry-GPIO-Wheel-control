#include <iostream>
#include <wiringPi.h>

using namespace std;

#define	enablePin	1
#define	left1Pin	4
#define	left2Pin	5
#define	right1Pin	21
#define	right2Pin	22


#pragma region classes
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


	void ChangeMode(bool isOut) {
		if (out == isOut) return;
	
		out = isOut;
		TurnOn();
	}

	void TurnOn() {
		pinMode(portNumber, out? OUTPUT : INPUT);

		isOn = true;
		cout << "DEBUG: " << "gpio mode " << portNumber << " " << (out ? "out" : "in") << endl;
	}

	void ToggleHigh() {
		SetHigh(!high);
	}

	void SetHigh(bool high) {
		digitalWrite(portNumber, high? HIGH : LOW);

		this->high = high;

		cout << "DEBUG: " << "gpio write " << portNumber << " " << (high ? "1" : "0") << endl;
	}

	long PulseIn(){
		if (!out) 
			return pulseIn(portNumber, !high);
	
		cout << "Port " << portNumber << "is set as out, but trying to read pulse from it. returning 0";
		return 0;
	}

	int AnalogRead() {
		if (!out)
			return analogRead(pin);

		cout << "Port " << portNumber << "is set as out, but trying to read analog data from it. returning 0";
		return 0;
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

#pragma endregion this is where all the classes lie

int notMain()
//int main(void)
{
	wiringPiSetup();

	WheelControl* wheelControl;
	cout << "Starting the Engine" << endl;
	Port* A1 = new Port(left1Pin, true, false);
	Port* A2 = new Port(left2Pin, true, false);
	HBridgePair* A = new HBridgePair(A1, A2);

	Port* B1 = new Port(right1Pin, true, false);
	Port* B2 = new Port(right2Pin, true, false);
	HBridgePair* B = new HBridgePair(B1, B2);

	wheelControl = new WheelControl(enablePin, A, B);
	wheelControl->Initialize();

	while (true) {
		GetInput(wheelControl);
	}

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

class DistanceMeasurer {
protected:
	Port* port;

	DistanceMeasurer(Port* port) {
		this->port = port;
	}
public:
	virtual float GetDistance() {
		cout << "GetDistance was not overriden for port " << port;
	}
};

class UltrasoundSensor : DistanceMeasurer {
public:
	UltrasoundSensor(Port* port) : DistanceMeasurer(port){}

	float GetDistance() {
		port->ChangeMode(true);
		port->ToggleHigh();
		delayMicroseconds(10);
		port->ToggleHigh();
		port->ChangeMode(false);
		long duration = port->PulseIn();
		float distance = duration / 58.0; //sound speed in cm/s

		return distance; //cm
	}
};

class IRSensor : DistanceMeasurer {
public:
	IRSensor(Port* port) : DistanceMeasurer(port) {}

	float GetDistance() {
		int val = port->AnalogRead();
		float voltage = val * (5.0 / 1023.0);
		float distance = 13.0 * pow(voltage, -1.10); //found these somewhere, might need to adjust
		return distance; //cm
	}
}; 

class DistanceMatrix {
private:
	DistanceMeasurer* measurers;
	int totalSize;

public:
	DistanceMatrix(DistanceMeasurer measurers[]) {
	  this->measurers = measurers;
	  totalSize = sizeof(measurers);
	}

	int count() {
		return totalSize;
	}

	float* getDistances() {
		float* distances = new float[totalSize];

		for (int i = 0; i < totalSize; i++) {
			distances[i] = measurers[i].GetDistance();
		}

		return distances;
	}
};