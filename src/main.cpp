#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <optional>
#include <chrono>
#include <iomanip>
#include <thread>

#include "hardware_driver.hpp"
#include "hardware_interface.hpp"

namespace fs = std::filesystem;

int main(int argc, char const *argv[])
{
    // constant param_folder
    std::string param_folder = "../params";
    fs::path param = fs::current_path() / param_folder;

    /***** log *****/
    // get the current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d-%H%M%S");
    // create log folder named by current time
    fs::path log_folder = fs::current_path() / "logs" / ss.str();
    fs::create_directories(log_folder);
    // using csv format to save log data
    fs::path log_file_name = log_folder / "log.csv";

    /***** Controller *****/
    std::string config_file_dir = param / "driver_params.yaml";
    
    int domain_id;
    std::string interface_name;
    if(argc < 2) // local loopback
    {
        domain_id = 1;
        interface_name = "lo";
    }
    else // specified interface
    {
        domain_id = 0;
        interface_name = argv[1];
    }
    HardwareDriver *hardware_driver = new HardwareDriver(domain_id, interface_name);
    // load parameter from .json file
    hardware_driver->loadParam(config_file_dir);

    // 开启外设通信
    hardware_driver->initDDSPubSub();
    std::chrono::milliseconds duration(1000); // 1000ms, wait for peripheral threads to start
    std::this_thread::sleep_for(duration);

    // start control loop
    hardware_driver->start();

    return 0;
}