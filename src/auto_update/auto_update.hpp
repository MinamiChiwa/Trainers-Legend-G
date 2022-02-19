#pragma once

#include <cpprest/http_client.h>
#include <filesystem>
#include <optional>

namespace AutoUpdate
{
	struct IAutoUpdateService
	{
		virtual ~IAutoUpdateService();

		// 若返回空值表示没有可更新的版本或者无有效的 Release，返回值表示已经完成更新，其值为最新的版本
		virtual Concurrency::task<std::optional<std::string>> CheckAndUpdate(const std::string& currentVersion, const std::filesystem::path& dest) = 0;
	};

	std::unique_ptr<IAutoUpdateService> CreateAutoUpdateService(std::string_view source, std::string path);
}
