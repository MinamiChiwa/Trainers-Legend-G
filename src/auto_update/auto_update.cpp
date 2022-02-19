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

			Concurrency::task<std::optional<ReleaseInfo>> GetLatestRelease() override;

		private:
			utility::string_t m_Path;
		};

		GiteeAutoUpdateService::GiteeAutoUpdateService(const std::string& path)
			: m_Path(utility::conversions::to_string_t(path))
		{
		}

		Concurrency::task<std::optional<ReleaseInfo>> GiteeAutoUpdateService::GetLatestRelease()
		{
			const auto uri = std::format(U("https://gitee.com/api/v5/repos/{}/releases/latest"), m_Path);
			std::wprintf(L"Getting %s...\n", uri.c_str());
			web::http::client::http_client client(uri);

			return client.request(web::http::methods::GET).then([](const web::http::http_response& resp)
				{
					return resp.extract_json();
				}).then([=](const web::json::value& respValue) -> std::optional<ReleaseInfo>
					{
						const auto& respValueObj = respValue.as_object();
						const auto tagNameIter = respValueObj.find(U("tag_name"));
						if (tagNameIter == respValueObj.end())
						{
							return std::nullopt;
						}
						const auto& latestVersionTag = tagNameIter->second.as_string();
						const auto& assetsArray = respValueObj.at(U("assets")).as_array();
						for (const auto& asset : assetsArray)
						{
							const auto& assetObj = asset.as_object();
							const auto assetNameIter = assetObj.find(U("name"));
							if (assetNameIter != assetObj.end() && assetNameIter->second.as_string().ends_with(U(".zip")))
							{
								return ReleaseInfo{
									.Version = latestVersionTag,
									.Uri = assetObj.at(U("browser_download_url")).as_string(),
									.Comment = respValueObj.at(U("body")).as_string(),
								};
							}
						}

						return std::nullopt;
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

	Concurrency::task<void> DownloadFile(const utility::string_t& uri, const std::filesystem::path& dest)
	{
		const auto stream = Concurrency::streams::fstream::open_ostream(dest).get();
		web::http::client::http_client client(utility::conversions::to_string_t(uri));

		return client.request(web::http::methods::GET).then([=](const web::http::http_response& resp)
			{
				return resp.body().read_to_end(stream.streambuf());
			}).then([=](std::size_t)
				{
					stream.close();
				});
	}
}
