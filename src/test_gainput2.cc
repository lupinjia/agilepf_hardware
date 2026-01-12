#include <gainput/gainput.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <module/gamepad_interface.h>
#include <spdlog/spdlog.h>
namespace chrono = std::chrono;
using std::cout;
using std::endl;

int main()
{
	GamepadData gamepad_data;
	GamepadInterface gamepad_interface(gamepad_data);
	gamepad_interface.init();

	while (true)
	{
		gamepad_interface.run();
		if(gamepad_data.b == 1)
			spdlog::info("B button pressed!");
		// spdlog::info("--------------------Gamepad values--------------------");
		// cout << "left_stick_x value: " << gamepad_data.lx;
		// cout << ", left_stick_y value: " << gamepad_data.ly;
		// cout << ", right_stick_x value: " << gamepad_data.rx;
		// cout << ", right_stick_y value: " << gamepad_data.ry;
		// cout << endl;
		// cout << "start_button value: " << gamepad_data.start.pressed;
		// cout << ", select_button value: " << gamepad_data.select.pressed;
		// cout << endl;
		// cout << "left_button value: " << device->GetBool(46);
		// cout << ", right_button value: " << device->GetBool(47);
		// cout << ", up_button value: " << device->GetBool(48);
		// cout << ", down_button value: " << device->GetBool(49);
		// cout << endl;
		// cout << "a_button value: " << device->GetBool(50);
		// cout << ", b_button value: " << device->GetBool(51);
		// cout << ", x_button value: " << device->GetBool(52);
		// cout << ", y_button value: " << device->GetBool(53);
		// cout << endl;
		// cout << "l1_button value: " << device->GetBool(54);
		// cout << ", r1_button value: " << device->GetBool(55);
		// cout << endl;
		// cout << endl;
		std::this_thread::sleep_for(chrono::milliseconds(10));
	}
}

