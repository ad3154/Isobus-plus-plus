#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_file_server_client.hpp"

#include "console_logger.cpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <iterator>
#include <memory>

static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

void update_CAN_network(void *)
{
	isobus::CANNetworkManager::CANNetwork.update();
}

enum class ExampleStateMachineState
{
	OpenFile,
	WaitForFileToBeOpen,
	WriteFile,
	WaitForFileWrite,
	CloseFile,
	ExampleComplete
};

int main()
{
	std::signal(SIGINT, signal_handler);
	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug);
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
#endif

	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	// Consider customizing these values to match your device
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	std::vector<isobus::NAMEFilter> fsNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::FileServerOrPrinter));
	fsNameFilters.push_back(testFilter);

	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto TestPartnerFS = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, fsNameFilters);
	auto TestFileServerClient = std::make_shared<isobus::FileServerClient>(TestPartnerFS, TestInternalECU);

	TestFileServerClient->initialize(true);

	ExampleStateMachineState state = ExampleStateMachineState::OpenFile;
	std::string fileNameToUse = "FSExampleFile.txt";
	std::uint8_t fileHandle = isobus::FileServerClient::INVALID_FILE_HANDLE;
	const std::string fileExampleContents = "This is an example file! Visit us on Github https://github.com/Open-Agriculture/AgIsoStack-plus-plus";

	while (running)
	{
		// A little state machine to run our example.
		// Most functions on FS client interface are async, and can take a variable amount of time to complete, so
		// you will need to have some kind of stateful wrapper to manage your file operations.
		// This is essentially unavoidable, as interacting with files over the bus is a fairly involved process.
		switch (state)
		{
			// Let's open a file
			case ExampleStateMachineState::OpenFile:
			{
				if (TestFileServerClient->open_file(fileNameToUse, true, true, isobus::FileServerClient::FileOpenMode::OpenFileForReadingAndWriting, isobus::FileServerClient::FilePointerMode::AppendMode))
				{
					state = ExampleStateMachineState::WaitForFileToBeOpen;
				}
			}
			break;

			// While the interface tries to open the file, we wait and poll to see if it is open.
			case ExampleStateMachineState::WaitForFileToBeOpen:
			{
				// When we get a valid file handle, that means the file has been opened and is ready to be interacted with
				fileHandle = TestFileServerClient->get_file_handle(fileNameToUse);
				if (isobus::FileServerClient::INVALID_FILE_HANDLE != fileHandle)
				{
					state = ExampleStateMachineState::WriteFile;
				}
			}
			break;

			case ExampleStateMachineState::WriteFile:
			{
				if (TestFileServerClient->write_file(fileHandle, reinterpret_cast<const std::uint8_t *>(fileExampleContents.data()), fileExampleContents.size()))
				{
					state = ExampleStateMachineState::WaitForFileWrite;
				}
			}
			break;

			case ExampleStateMachineState::WaitForFileWrite:
			{
				if (isobus::FileServerClient::FileState::FileOpen == TestFileServerClient->get_file_state(fileHandle))
				{
					// If the file is back in the open state, then writing is done
					state = ExampleStateMachineState::CloseFile;
				}
			}
			break;

			// Let's clean up, and close the file.
			case ExampleStateMachineState::CloseFile:
			{
				if (TestFileServerClient->close_file(TestFileServerClient->get_file_handle(fileNameToUse)))
				{
					state = ExampleStateMachineState::ExampleComplete;
				}
			}
			break;

			// The example is complete! Do nothing until the user exits with ctrl+c
			default:
			case ExampleStateMachineState::ExampleComplete:
			{
			}
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	isobus::CANHardwareInterface::stop();
	return 0;
}
