#include "properties.hpp"

#include <gsl/gsl>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "io.hpp"
#include "com.hpp"
#include "string.hpp"

namespace utils::properties
{
	namespace
	{
		typedef rapidjson::GenericDocument<rapidjson::UTF16<>> WDocument;
		typedef rapidjson::GenericValue<rapidjson::UTF16<>> WValue;
		typedef rapidjson::GenericStringBuffer<rapidjson::UTF16<>> WStringBuffer;

		const std::filesystem::path get_properties_file()
		{
			static const auto props = get_appdata_path() / "user" / "properties.json";
			return props;
		}

		WDocument load_properties()
		{
			WDocument default_doc{};
			default_doc.SetObject();

			std::wstring data;
			const auto& props = get_properties_file();
			if (!io::read_file(props, &data))
			{
				return default_doc;
			}

			WDocument doc{};
			const rapidjson::ParseResult result = doc.Parse(data);
			if (!result || !doc.IsObject())
			{
				return default_doc;
			}

			return doc;
		}

		void store_properties(const WDocument& doc)
		{
			WStringBuffer buffer{};
			rapidjson::Writer<WStringBuffer, WDocument::EncodingType, rapidjson::UTF16<>>
				writer(buffer);
			doc.Accept(writer);

			const std::wstring json(buffer.GetString(), buffer.GetLength());

			const auto& props = get_properties_file();
			io::write_file(props, json);
		}
	}

	std::filesystem::path get_appdata_path()
	{
		PWSTR path;
		if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
		{
			throw std::runtime_error("Failed to read APPDATA path!");
		}

		auto _ = gsl::finally([&path]()
		{
			CoTaskMemFree(path);
		});

		static auto appdata = std::filesystem::path(path) / "xlabs";
		return appdata;
	}

	std::unique_lock<named_mutex> lock()
	{
		static named_mutex mutex{"xlabs-properties-lock"};
		std::unique_lock<named_mutex> lock{mutex};
		return lock;
	}

	std::optional<std::wstring> load(const std::wstring& name)
	{
		const auto _ = lock();
		const auto doc = load_properties();

		if (!doc.HasMember(name))
		{
			return {};
		}

		const auto& value = doc[name];
		if (!value.IsString())
		{
			return {};
		}

		return {std::wstring{value.GetString(), value.GetStringLength()}};
	}

	void store(const std::wstring& name, const std::wstring& value)
	{
		const auto _ = lock();
		auto doc = load_properties();

		while (doc.HasMember(name))
		{
			doc.RemoveMember(name);
		}

		WValue key{};
		key.SetString(name, doc.GetAllocator());

		WValue member{};
		member.SetString(value, doc.GetAllocator());

		doc.AddMember(key, member, doc.GetAllocator());

		store_properties(doc);
	}
}
