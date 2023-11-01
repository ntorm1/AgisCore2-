export module AgisFileUtils;

import <expected>;
import <filesystem>;


import AgisError;


export namespace Agis
{

enum class FileType
{
	CSV
};


std::expected<FileType, AgisException> get_file_type(std::string file_path)
{
	if (!std::filesystem::exists(file_path))
	{
		return std::unexpected(AgisException("File does not exist"));
	}
	if (file_path.ends_with(".csv"))
	{
		return FileType::CSV;
	}
	return std::unexpected(AgisException("File type not supported"));
}

}