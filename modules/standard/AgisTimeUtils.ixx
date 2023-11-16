module;

#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include <iomanip>
#include <chrono>


export module AgisTimeUtils;

import <string>;
import <expected>;
import <sstream>;

import AgisError;


namespace Agis
{

	export std::expected<long long, AgisException>
    str_to_epoch(
		std::string const& date_string,
		std::string const& dt_format)
	{
		std::tm timeStruct = {};
		std::istringstream iss(date_string);
		iss >> std::get_time(&timeStruct, dt_format.c_str());

		std::time_t utcTime = std::mktime(&timeStruct);

		// Convert to std::chrono::time_point
		std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::from_time_t(utcTime);

		// Get the epoch time in nanoseconds
		return std::chrono::duration_cast<std::chrono::nanoseconds>(timePoint.time_since_epoch()).count();
	}

    export AGIS_API std::expected<std::string, AgisException>
    epoch_to_str(
        long long epochTime,
        const std::string& formatString)
    {
        try {
            // Convert the epoch time from nanoseconds to seconds
            std::chrono::nanoseconds duration(epochTime);
            std::chrono::seconds epochSeconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

            // Convert the epoch time to std::time_t
            std::time_t epoch = epochSeconds.count();

            // Convert std::time_t to std::tm
            std::tm timeStruct;
            gmtime_s(&timeStruct, &epoch); // Use gmtime_s for safer handling

            // Format the time struct to a string
            std::stringstream ss;
            ss << std::put_time(&timeStruct, formatString.c_str());
            return ss.str();
        }
        catch (std::exception& e) {
            return std::unexpected(AgisException(e.what()));
        }
    }

}