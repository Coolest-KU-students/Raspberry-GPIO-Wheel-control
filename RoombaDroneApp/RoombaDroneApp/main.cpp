#include <iostream>
#include <fstream>
#include <signal.h>

#include <wiringPi.h>          // to control Raspberry Pi digital pins
#include "include/rplidar.h"   // RPLidar standard SDK
#include "src/datetime.h"      // to have current time for logging
#include <algorithm>           // to sort arrays

using namespace rp::standalone::rplidar;   // for RPLidar
using namespace jed_utils;                 // for datetime
using namespace std;

// ------------- Movement-related --------------- //

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
		} else {
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

// --------------- Lidar-related ---------------- //

int distanceToObstacleInFrontLimit = 3000; // mm
int rideDuration = 2; // s

datetime movementStart;
datetime defaultTime = datetime(2000, 1, 1, 0, 0, 0);

bool checkLidarHealth(RPlidarDriver* driver) {
	u_result opResult;
	rplidar_response_device_health_t healthinfo;

	opResult = driver->getHealth(healthinfo);
	if (IS_OK(opResult)) {
		if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
			std::cout << jed_utils::datetime().to_string() << " Lidar internal error detected. Please reboot the device to retry.\n";
			return false;
		}
		else {
			return true;
		}
	}
	else {
		std::cout << jed_utils::datetime().to_string() << " Cannot retrieve the Lidar health code: " << opResult << std::endl;
		return false;
	}
}

bool ctrl_c_pressed;
void ctrlc(int) {
	ctrl_c_pressed = true;
}

void onFinished(RPlidarDriver* driver) {
	RPlidarDriver::DisposeDriver(driver);
	driver = NULL;
}

void moveToLeft(WheelControl* wheelControl) {
	movementStart.operator=(datetime());
	wheelControl->TurnLeft();
}

void noMovement() {
	movementStart = datetime(2000, 1, 1, 0, 0, 0);
}

void moveToRight(WheelControl* wheelControl) {
	movementStart.operator=(datetime());
	wheelControl->TurnRight();
}

void checkMovement() {
	int dur = (datetime() - movementStart).get_seconds();
	if (!(dur == 0) && (dur > rideDuration)) {
		noMovement();
	}
}

double median(int arr[], int size) {
	std::sort(arr, arr + size);
	if (size % 2 != 0)
		return (double)arr[size / 2];
	return (double)(arr[(size - 1) / 2] + arr[size / 2]) / 2.0;
}

// --------------- End of blocks ---------------- //

int main(void)
{
	const char*  comPath = "/dev/ttyUSB0"; // default Lidar communication port path
	_u32         baudrate = 256000;        // default communication speed between Lidar and Raspberry Pi
	u_result     opResult;                 // operation result
	bool         connectSuccess = false;
	int          connectionTimeout = 0;

	std::cout << jed_utils::datetime().to_string() << " App started\n";

	// create the driver instance
	RPlidarDriver* driver = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);

	if (!driver) {
		std::cout << jed_utils::datetime().to_string() << " Insufficent memory, exit.\n";
		exit(-2);
	}

	std::cout << jed_utils::datetime().to_string() << " Connecting to Lidar\n";

	rplidar_response_device_info_t devInfo;

	// check for the Lidar and connect to it
	// if it doesn't find the device in 15 minutes - throw an error and end the program
	while (connectionTimeout < 900) {
		if (IS_OK(driver->connect(comPath, baudrate))) {
			opResult = driver->getDeviceInfo(devInfo);
			if (IS_OK(opResult)) {
				std::cout << jed_utils::datetime().to_string() << " Connected to Lidar\n";
				connectSuccess = true;
				break;
			}
		}
		std::cout << jed_utils::datetime().to_string() << " Failed connecting to Lidar. " << connectionTimeout << "\\\900 \n";
		delay(1000);
		connectionTimeout++;
	}

	if (!connectSuccess) {
		std::cout << jed_utils::datetime().to_string() << " Cannot bind to the pre-defined serial port: " << comPath << ", exit\n";
		onFinished(driver);
	}

	// check Lidar health
	if (!checkLidarHealth(driver)) {
		onFinished(driver);
	}

	// create detection of ctrl + c pressed
	signal(SIGINT, ctrlc);

	// open filestream to results.txt (located in the same folder with .out file)
	std::fstream resultFile;

	connectionTimeout = 0;
	connectSuccess = false;

	while (connectionTimeout < 60) {
		resultFile.open("results.txt", std::fstream::out);
		if (resultFile.is_open()) {
			std::cout << jed_utils::datetime().to_string() << " Opened results.txt file\n";
			connectSuccess = true;
			break;
		}
		std::cout << jed_utils::datetime().to_string() << " Failed to open results.txt file. " << connectionTimeout << "\\\60 \n";
		delay(1000);
		connectionTimeout++;
	}

	if (!connectSuccess) {
		std::cout << jed_utils::datetime().to_string() << " Failed to open results.txt file, exit.\n";
		onFinished(driver);
	}

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

	// start scanning
	driver->startMotor();
	driver->startScan(0, 1);

	std::cout << jed_utils::datetime().to_string() << " Detection started\n";

	int lastScanData[360];

	int leftSideObstacles[31];
	int leftSideArrayPos = 0;
	int rightSideObstacles[31];
	int rightSideArrayPos = 0;

	int obstacleTooClose = 0;

	for (int i = 0; i < 360; i++) {
		lastScanData[i] = 0;
	}

	while (1) {
		size_t count = 8192;
		rplidar_response_measurement_node_hq_t nodes[count];

		obstacleTooClose = 0;
		leftSideArrayPos = 0;
		rightSideArrayPos = 0;

		for (int i = 0; i < 31; i++) {
			leftSideObstacles[i] = 0;
			rightSideObstacles[i] = 0;
		}

		opResult = driver->grabScanDataHq(nodes, count);

		if (IS_OK(opResult) || opResult == RESULT_OPERATION_TIMEOUT) {
			driver->ascendScanData(nodes, count);

			int results[360]; // contains one 360 spin - array position is degree and value is distance

			// if value will not be set it will be 0 - no obstacle
			for (int i = 0; i < 360; i++) {
				results[i] = 0;
			}

			for (int pos = 0; pos < (int)count; ++pos) {
				if (nodes[pos].quality != 0) {
					// lidar returns data in hundredths (ex. 1,55 deg), so the detected data is rounded to have better handling of data amount
					int deg = round(nodes[pos].angle_z_q14 * 90.f / (1 << 14));
					deg = deg == 360 ? 0 : deg;

					// note: distance is detected in millimeters
					int dist = round((nodes[pos].dist_mm_q2 / 4.0f)) - 50; // -5 cm, because it includes the body size in detection

					// if multiple degrees exist on a scan data - value with the closest distance is selected
					results[deg] = results[deg] == 0          // if not initialized
								  	  ? dist                  // assign value
									  : results[deg] < dist   // if current lower than offered
									      ? results[deg]      // leave current
								          : dist;             // else assign
				}
			}

			// initiation of json arrays
			std::string detectedData = "\"ValueArray\": \"[";
			std::string movementData = "\"MovementArray\": \"[";

			for (int i = 0; i < 360; i++) {
				// if distance is less than 10 mm - do not save
				results[i] = (results[i] < 10) ? 0 : results[i];

				// technical distance limit - 0.5 meters (failsafe for any incorrect information)
				results[i] = (results[i] > 5000) ? 5000 : results[i];

				// save last scan data
				lastScanData[i] = results[i];

				// detect distances to obstacles
				if (results[i] < distanceToObstacleInFrontLimit && results[i] != 0) {
					if (i == 0) {
						rightSideObstacles[rightSideArrayPos] = results[i];
						leftSideObstacles[leftSideArrayPos] = results[i];
						rightSideArrayPos++;
						leftSideArrayPos++;
					}

					if (i > 0 && i <= 30) {
						rightSideObstacles[rightSideArrayPos] = results[i];
						rightSideArrayPos++;
					}

					if (i >= 330 && i < 360) {
						leftSideObstacles[leftSideArrayPos] = results[i];
						leftSideArrayPos++;
					}
				}

				// ---------- Writing results into strings ---------- //

				// convert result int to string
				std::string resultDist = std::to_string(results[i]);

				// remove last digit to make the result in centimeters
				if (results[i] != 0) {
					resultDist.pop_back();
				}

				// add result to resuls array (first element without space)
				detectedData += (i != 0) ? (" " + resultDist + ",") : (resultDist + ",");
			}

			int leftSideOnlyObstacles[++leftSideArrayPos];
			int rightSideOnlyObstacles[++rightSideArrayPos];

			for (int i = 0; i < leftSideArrayPos; i++) { leftSideOnlyObstacles[i] = leftSideObstacles[i]; }

			for (int i = 0; i < rightSideArrayPos; i++) { rightSideOnlyObstacles[i] = rightSideObstacles[i]; }

			// get medians of the obstacle distances
			int leftSideMedian = median(leftSideOnlyObstacles, leftSideArrayPos);
			int rightSideMedian = median(rightSideOnlyObstacles, rightSideArrayPos);

			// turn left if there are obstacles on the right side, but none on the left
			if (leftSideMedian == 0 && rightSideMedian != 0) { moveToLeft(wheelControl); }

			// turn right if there are obstacles on the left side, but none on the right
			if (leftSideMedian != 0 && rightSideMedian == 0) { moveToRight(wheelControl); }

			if (leftSideMedian != 0 && rightSideMedian != 0) {
				// turn right if there are closer obstacles on the left side
				if (leftSideMedian < rightSideMedian) { moveToRight(wheelControl); }

				// turn left if there are closer obstacles on the right side
				if (leftSideMedian > rightSideMedian) { moveToLeft(wheelControl); }

				// very hard case, but if there is equally the same median distance - just turn left
				if (leftSideMedian == rightSideMedian) { moveToLeft(wheelControl); }
			}

			checkMovement();

			// remove last comma
			detectedData.pop_back();
			movementData.pop_back();

			// add ending to the arrays and json object
			detectedData += "]\"";
			movementData += "]\" }";

			// write detected data to results.txt
			resultFile << "{ \"Datetime\": \"" << jed_utils::datetime().to_string() << "\", " << detectedData << ", " << movementData << "\n";

			GetInput(wheelControl);
		} else {
			std::cout << jed_utils::datetime().to_string() << " Lidar failed to get data, error code: " << opResult << "\n";
		}
		if (ctrl_c_pressed) {
			break;
		}
	}

	// stop scanning
	driver->stop();
	driver->stopMotor();

	// end program
	std::cout << jed_utils::datetime().to_string() << " App cancellation initiated\n";
	onFinished(driver);
	resultFile.close();

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
