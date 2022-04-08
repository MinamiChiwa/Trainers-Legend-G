#pragma once

#include <cpprest/http_client.h>
#include <filesystem>
#include <optional>

namespace AutoUpdate
{
	struct ReleaseInfo
	{
		utility::string_t Version;
		utility::string_t Uri;
		utility::string_t Comment;
	};

	struct IAutoUpdateService
	{
		virtual ~IAutoUpdateService();

		virtual Concurrency::task<std::optional<ReleaseInfo>> GetLatestRelease() = 0;
	};

	std::unique_ptr<IAutoUpdateService> CreateAutoUpdateService(std::string_view source, std::string path);
	Concurrency::task<void> DownloadFile(const utility::string_t& uri, const std::filesystem::path& dest);
}
