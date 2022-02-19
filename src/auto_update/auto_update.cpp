#include "auto_update.hpp"

#include <format>
#include <cpprest/filestream.h>

namespace AutoUpdate
{
	IAutoUpdateService::~IAutoUpdateService()
	{
	}

	namespace
	{
		class GiteeAutoUpdateService : public IAutoUpdateService
		{
		public:
			explicit GiteeAutoUpdateService(const std::string& path);

			Concurrency::task<std::optional<std::string>> CheckAndUpdate(const std::string& currentVersion, const std::filesystem::path& dest) override;

		private:
			utility::string_t m_Path;
		};

		GiteeAutoUpdateService::GiteeAutoUpdateService(const std::string& path)
			: m_Path(utility::conversions::to_string_t(path))
		{
		}

		Concurrency::task<std::optional<std::string>> GiteeAutoUpdateService::CheckAndUpdate(const std::string& currentVersion, const std::filesystem::path& dest)
		{
			const auto uri = std::format(U("https://gitee.com/api/v5/repos/{}/releases/latest"), m_Path);
			std::wprintf(L"Getting %s...\n", uri.c_str());
			web::http::client::http_client client(uri);

			return client.request(web::http::methods::GET).then([](const web::http::http_response& resp)
				{
					return resp.extract_json();
				}).then([=](const web::json::value& respValue) -> Concurrency::task<std::optional<std::string>>
					{
						const auto& respValueObj = respValue.as_object();
						const auto tagNameIter = respValueObj.find(U("tag_name"));
						if (tagNameIter == respValueObj.end())
						{
							std::printf("No release found\n");
							return Concurrency::task<std::optional<std::string>>{[] { return std::nullopt; }};
						}
						const auto& latestVersionTag = tagNameIter->second.as_string();
						std::wprintf(L"Latest version is %s\n", latestVersionTag.c_str());
						if (utility::conversions::to_utf8string(latestVersionTag) == currentVersion)
						{
							return Concurrency::task<std::optional<std::string>>{[] { return std::nullopt; }};
						}
						const auto& assetsArray = respValueObj.at(U("assets")).as_array();
						for (const auto& asset : assetsArray)
						{
							const auto& assetObj = asset.as_object();
							if (assetObj.at(U("name")).as_string().ends_with(U(".zip")))
							{
								const auto stream = Concurrency::streams::fstream::open_ostream(dest).get();

								web::http::client::http_client client(assetObj.at(U("browser_download_url")).as_string());
								return client.request(web::http::methods::GET).then([=](const web::http::http_response& resp)
									{
										return resp.body().read_to_end(stream.streambuf());
									}).then([=](std::size_t)
										{
											stream.close();
											return std::optional(utility::conversions::to_utf8string(latestVersionTag));
										});
							}
						}

						return Concurrency::task<std::optional<std::string>>{[] { return std::nullopt; }};
					});
		}
	}

	std::unique_ptr<IAutoUpdateService> CreateAutoUpdateService(std::string_view source, std::string path)
	{
		if (source == "gitee")
		{
			return std::make_unique<GiteeAutoUpdateService>(std::move(path));
		}

		return nullptr;
	}
}
