module;

#include "AgisMacros.h"
#include <fstream>
#include <unordered_set>
#include <algorithm>

module AssetPrivateModule;

import AgisTimeUtils;

namespace Agis
{


//============================================================================
void
AssetPrivate::add_observer(UniquePtr<AssetObserver> observer) noexcept
{
	auto str_rep = observer->str_rep();
	if(!observers.contains(str_rep))
	{
		observers[std::move(str_rep)] = std::move(observer);
	}
}


//============================================================================
size_t
AssetPrivate::get_index(size_t row, size_t col) {
	return row * _cols + col;
}


//============================================================================
std::expected<bool, AgisException>
AssetPrivate::validate_headers()
{
	static const char* required_headers[] = {
		"open",
		"close",
	};
	// get lower case headers
	std::unordered_set<std::string> lower_headers;
	std::vector<std::string> headers_vec;
	for (auto header : this->_headers)
	{
		auto h_copy = header.first;
		std::transform(
			h_copy.begin(),
			h_copy.end(),
			h_copy.begin(),
			[](unsigned char c) { return std::tolower(c); });
		lower_headers.insert(h_copy);
		headers_vec.push_back(header.first);
	}
	// check if close and open are present
	for (auto& header : required_headers) 
	{
		auto it = lower_headers.find(header);
		if (it == lower_headers.end()) 
		{
			return std::unexpected(AgisException("Could not find header"));
		}
		else {
			// get the index of the item
			auto index = std::distance(lower_headers.begin(), it);
			if (*it == "open") 
			{
				this->_open_index = index;
			}
			else if (*it == "close") 
			{
				this->_close_index = index;
				this->_close_column = headers_vec[index];
			}
		}
	}
	return true;
}

//============================================================================
std::expected<bool, AgisException>
AssetPrivate::load_csv(
	std::string filename,
	std::string dt_format
	)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		return std::unexpected(AgisException("Could not open file " + filename));
	}

	// get row count
	this->_rows = 0;
	std::string line;
	while (std::getline(file, line)) {
		this->_rows++;
	}
	this->_rows--;
	file.clear();                 // Clear any error flags
	file.seekg(0, std::ios::beg);  // Move the file pointer back to the start

	// Parse headers
	if (std::getline(file, line)) {
		std::stringstream ss(line);
		std::string columnName;
		int columnIndex = 0;

		// Skip the first column (date)
		std::getline(ss, columnName, ',');
		while (std::getline(ss, columnName, ',')) {
			this->_headers[columnName] = columnIndex;
			columnIndex++;
		}
	}
	else {
		return std::unexpected(AgisException("Could not parse headers"));
	}
	// validate headers
	AGIS_ASSIGN_OR_RETURN(res, this->validate_headers());
	this->_cols = this->_headers.size();
	this->_data.resize(this->_rows * this->_cols, 0);
	this->_dt_index.resize(this->_rows);

	size_t row_counter = 0;
	while (std::getline(file, line))
	{
		std::stringstream ss(line);

		// First column is datetime
		std::string dateStr, columnValue;
		std::getline(ss, dateStr, ',');
		AGIS_ASSIGN_OR_RETURN(epoch, str_to_epoch(dateStr, dt_format));
		
		this->_dt_index[row_counter] = epoch;

		int col_idx = 0;
		while (std::getline(ss, columnValue, ','))
		{
			double value = std::stod(columnValue);
			size_t index = this->get_index(row_counter, col_idx); // Calculate the row-major index
			this->_data[index] = value;
			col_idx++;
		}
		row_counter++;
	}

	return true;
}


//============================================================================
std::expected<bool, AgisException> AssetPrivate::load_h5(
	H5::DataSet& dataset,
	H5::DataSpace& dataspace,
	H5::DataSet& datasetIndex,
	H5::DataSpace& dataspaceIndex)
{
	// Get the number of attributes associated with the dataset
	int numAttrs = dataset.getNumAttrs();
	// Iterate through the attributes to find the column names
	for (int i = 0; i < numAttrs; i++) {
		// Get the attribute at index i
		H5::Attribute attr = dataset.openAttribute(i);

		// Check if the attribute is a string type
		if (attr.getDataType().getClass() == H5T_STRING) {
			// Read the attribute as a string
			std::string attrValue;
			attr.read(attr.getDataType(), attrValue);

			// Store the attribute value as a column name
			this->_headers[attrValue] = static_cast<size_t>(i);
		}
	}
	AGIS_ASSIGN_OR_RETURN(res, this->validate_headers());
	// Get the number of rows and columns from the dataspace
	int numDims = dataspace.getSimpleExtentNdims();
	std::vector<hsize_t> dims(numDims);
	dataspace.getSimpleExtentDims(dims.data(), nullptr);
	_rows = dims[0];
	_cols = dims[1];
	// Allocate memory for the column-major array
	_data.resize(_rows * _cols, 0);
	dataset.read(_data.data(), H5::PredType::NATIVE_DOUBLE, dataspace);

	// Allocate memory for the array to hold the data
	_dt_index.resize(_rows, 0);
	datasetIndex.read(_dt_index.data(), H5::PredType::NATIVE_INT64, dataspaceIndex);
	return true;
}


//============================================================================
AssetPrivate::~AssetPrivate()
{
}

}