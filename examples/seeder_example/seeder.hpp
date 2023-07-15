//================================================================================================
/// @file seeder.hpp
///
/// @brief This is the definition of an example seeder application
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef SEEDER_HPP
#define SEEDER_HPP

#include "vt_application.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"

class Seeder
{
public:
	Seeder() = default;

	bool initialize();

	void terminate();

	void update();

private:
	std::unique_ptr<SeederVtApplication> VTApplication = nullptr;
	std::unique_ptr<isobus::DiagnosticProtocol> diagnosticProtocol = nullptr;
};

#endif // SEEDER_HPP
