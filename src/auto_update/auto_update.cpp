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
			explicit GiteeAutoUpdateService(const std::string& path)
				: m_Path(utility::conversions::to_string_t(path))
			{
			}

			Concurrency::task<std::optional<ReleaseInfo>> GetLatestRelease() override
			{
				const auto uri = std::format(U("https://gitee.com/api/v5/repos/{}/releases/latest"), m_Path);
				std::wprintf(L"Getting %ls...\n", uri.c_str());
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

		private:
			utility::string_t m_Path;
		};

		class GithubAutoUpdateService : public IAutoUpdateService
		{
		public:
			explicit GithubAutoUpdateService(const std::string& path)
				: m_Path(utility::conversions::to_string_t(path))
			{
			}

			Concurrency::task<std::optional<ReleaseInfo>> GetLatestRelease() override
			{
				if (m_Path.starts_with(U("http://")) || m_Path.starts_with(U("https://")))
				{
					// 使用转发 api，假定此 api 只会返回最新的 asset
					std::wprintf(L"Getting %ls...\n", m_Path.c_str());
					web::http::client::http_client client(m_Path);
					return client.request(web::http::methods::GET).then([](const web::http::http_response& resp)
						{
							return resp.extract_json();
						}).then([=](const web::json::value& respValue)->std::optional<ReleaseInfo>
							{
								const auto& latestRelease = respValue.as_object();
								const auto tagNameIter = latestRelease.find(U("tag_name"));
								if (tagNameIter == latestRelease.end())
								{
									return std::nullopt;
								}
								const auto& latestVersionTag = tagNameIter->second.as_string();
								const auto& assetsArray = latestRelease.at(U("assets")).as_array();
								for (const auto& asset : assetsArray)
								{
									const auto& assetObj = asset.as_object();
									const auto assetNameIter = assetObj.find(U("name"));
									if (assetNameIter != assetObj.end() && assetNameIter->second.as_string().ends_with(U(".zip")))
									{
										return ReleaseInfo{
											.Version = latestVersionTag,
											.Uri = assetObj.at(U("browser_download_url")).as_string(),
											.Comment = latestRelease.at(U("body")).as_string(),
										};
									}
								}

								return std::nullopt;
							});
				}
				else
				{
					// 直接使用 github api
					const auto uri = std::format(U("https://api.github.com/repos/{}/releases"), m_Path);
					std::wprintf(L"Getting %ls...\n", uri.c_str());
					web::http::client::http_client client(uri);
					return client.request(web::http::methods::GET).then([](const web::http::http_response& resp)
						{
							return resp.extract_json();
						}).then([=](const web::json::value& respValue)->std::optional<ReleaseInfo>
							{
								const auto& respValueArray = respValue.as_array();
								if (respValueArray.size() == 0)
								{
									return std::nullopt;
								}
								const auto& latestRelease = respValueArray.at(0).as_object();
								const auto tagNameIter = latestRelease.find(U("tag_name"));
								if (tagNameIter == latestRelease.end())
								{
									return std::nullopt;
								}
								const auto& latestVersionTag = tagNameIter->second.as_string();
								const auto& assetsArray = latestRelease.at(U("assets")).as_array();
								for (const auto& asset : assetsArray)
								{
									const auto& assetObj = asset.as_object();
									const auto assetNameIter = assetObj.find(U("name"));
									if (assetNameIter != assetObj.end() && assetNameIter->second.as_string().ends_with(U(".zip")))
									{
										return ReleaseInfo{
											.Version = latestVersionTag,
											.Uri = assetObj.at(U("browser_download_url")).as_string(),
											.Comment = latestRelease.at(U("body")).as_string(),
										};
									}
								}

								return std::nullopt;
							});
				}
			}

		private:
			utility::string_t m_Path;
		};

		class MockLocalFileAutoUpdateService : public IAutoUpdateService
		{
		public:
			explicit MockLocalFileAutoUpdateService(std::filesystem::path path)
				: m_Path(std::move(path))
			{
			}

			Concurrency::task<std::optional<ReleaseInfo>> GetLatestRelease() override
			{
				return Concurrency::task<std::optional<ReleaseInfo>>([&]() -> std::optional<ReleaseInfo>
					{
						return ReleaseInfo
						{
							.Version = U("latest"),
							.Uri = std::format(U("mock://{}"), m_Path.wstring()),
							.Comment = U("Mock test"),
						};
					});
			}

		private:
			std::filesystem::path m_Path;
		};
	}

	std::unique_ptr<IAutoUpdateService> CreateAutoUpdateService(std::string_view source, std::string path)
	{
		if (source == "gitee")
		{
			return std::make_unique<GiteeAutoUpdateService>(path);
		}

		if (source == "github")
		{
			return std::make_unique<GithubAutoUpdateService>(path);
		}

		if (source == "mock")
		{
			return std::make_unique<MockLocalFileAutoUpdateService>(std::move(path));
		}

		return nullptr;
	}

	Concurrency::task<void> DownloadFile(const utility::string_t& uri, const std::filesystem::path& dest)
	{
		const auto stream = Concurrency::streams::fstream::open_ostream(dest).get();
		const web::uri uriObj = uri;
		if (uriObj.scheme() == U("mock"))
		{
			std::filesystem::copy_file(uriObj.path(), dest, std::filesystem::copy_options::overwrite_existing);
			return Concurrency::task<void>([] {});
		}
		web::http::client::http_client client(uriObj);

		return client.request(web::http::methods::GET).then([=](const web::http::http_response& resp)
			{
				return resp.body().read_to_end(stream.streambuf());
			}).then([=](std::size_t)
				{
					stream.close();
				});
	}
}
