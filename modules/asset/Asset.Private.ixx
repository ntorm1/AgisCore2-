export module AssetModule:AssetPrivateModule;

import <vector>;
import <unordered_map>;
import <expected>;
import <string>;


import AgisError;


namespace Agis 
{


export struct AssetPrivate
{	
	size_t _rows;
	size_t _cols;
	size_t _open_index;
	size_t _close_index;
	size_t _current_index = 0;
	std::vector<long long> _dt_index;
	std::vector<double> _data;
	double* _data_ptr;
	std::unordered_map<std::string, size_t> _headers;


	size_t get_index(size_t row, size_t col);
	std::expected<bool, AgisException> validate_headers();
	std::expected<bool, AgisException> load_csv(std::string filename, std::string dt_format);

	AssetPrivate()
	{
		_rows = 0;
		_cols = 0;
		_open_index = 0;
		_close_index = 0;
	}

};


}