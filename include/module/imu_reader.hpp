#ifndef IMU_READER_HPP
#define IMU_READER_HPP
#include <iostream>
#include "msg/imu_msg.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#ifdef __cplusplus
extern "C"{
#endif
#include <poll.h>
#include "ch_serial.h"

#define GRA_ACC     (9.8)
#define DEG_TO_RAD  (0.01745329)
#define BUF_SIZE    (1024)
#ifdef __cplusplus
}
#endif

using namespace std::chrono_literals;
using namespace std;
static hipnuc_raw_t raw;

class ImuReader
{
public:
		int fd = 0;
		uint8_t buf[BUF_SIZE] = {0};
		ImuReader()
		{
			serial_port = "/dev/ttyUSB0";
			baud_rate = 921600;
			memset(&imu_data, 0, sizeof(ImuMsg));
			
			spdlog::info("Initializing IMU Reader...");
			spdlog::info("imu_serial port: {}", serial_port);
			spdlog::info("imu_baud_rate: {}", baud_rate);

			fd = open_serial(serial_port, baud_rate);
		}

		void imu_read(void)
		{
			struct pollfd p;
			p.fd = fd;
			p.events = POLLIN;
			
			int rpoll = poll(&p, 1, 5);

			if(rpoll == 0)
				return ;

			int n = read(fd, buf, sizeof(buf));

			for(int i = 0; i < n; i++)
			{
				int rev = ch_serial_input(&raw, buf[i]);
				
				if(rev)
				{
					imu_data.quat_w = raw.imu.quat[0];
					imu_data.quat_x = raw.imu.quat[1];	
					imu_data.quat_y = raw.imu.quat[2];
					imu_data.quat_z = raw.imu.quat[3];
					imu_data.ang_vel_x = raw.imu.gyr[0] * DEG_TO_RAD;
					imu_data.ang_vel_y = raw.imu.gyr[1] * DEG_TO_RAD;
					imu_data.ang_vel_z = raw.imu.gyr[2] * DEG_TO_RAD;
					imu_data.lin_acc_x = raw.imu.acc[0] * GRA_ACC;
					imu_data.lin_acc_y = raw.imu.acc[1] * GRA_ACC;
					imu_data.lin_acc_z = raw.imu.acc[2] * GRA_ACC;
					imu_data.roll = raw.imu.eul[0] * DEG_TO_RAD;
					imu_data.pitch = raw.imu.eul[1] * DEG_TO_RAD;
					imu_data.yaw = raw.imu.eul[2] * DEG_TO_RAD;
				}
			}

			memset(buf,0,sizeof(buf));
		}

		ImuMsg imu_data;

private: 
		int open_serial(std::string port, int baud)
		{
			const char* port_device = port.c_str();
			int fd = open(port_device, O_RDWR | O_NOCTTY | O_NONBLOCK);
		
			if(fd == -1)
			{
				perror("unable to open serial port");
				exit(0);
			}
			
			if(fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
				cout << "fcntl failed" << "\n" << endl;
			else
				fcntl(fd, F_SETFL, O_NONBLOCK);

			struct termios options;
			memset(&options, 0, sizeof(options));
			tcgetattr(fd, &options);
	
			switch(baud)
			{
				case 115200:
					cfsetispeed(&options, B115200);
					cfsetospeed(&options, B115200);
				break;
				case 460800:
					cfsetispeed(&options, B460800);
					cfsetospeed(&options, B460800);
				break;
				case 921600:
					cfsetispeed(&options, B921600);
					cfsetospeed(&options, B921600);
				break;
			}

			options.c_cflag &= ~PARENB; 
			options.c_cflag &= ~CSTOPB; 
			options.c_cflag &= ~CSIZE;  
			options.c_cflag |= HUPCL;   
			options.c_cflag |= CS8;     
			options.c_cflag &= ~CRTSCTS; 
			options.c_cflag |= CREAD | CLOCAL; 

			options.c_iflag &= ~(IXON | IXOFF | IXANY); 
			options.c_iflag &= ~(INLCR|ICRNL); 

			options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

			options.c_oflag &= ~OPOST; 
			options.c_oflag &= ~(ONLCR|OCRNL); 

			options.c_cc[VMIN] = 0;  
			options.c_cc[VTIME] = 0; 
			tcsetattr(fd, TCSANOW, &options);

			return fd;
		}

		std::string serial_port;
		int baud_rate;
};

#endif