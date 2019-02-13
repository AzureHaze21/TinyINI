#ifndef _TINYINI_H_
#define _TINYINI_H_

#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>

#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>

class TinyIni
{
public:
	class KeyValues
	{
		friend class TinyIni;
	private:
		std::map<std::wstring, std::wstring> data{};
	public:
		auto operator[](const wchar_t *key) { return key == nullptr ? L"" : data[key]; }
	};
private:
	std::wifstream filestream;

	class Sections
	{
		friend class TinyIni;
	private:
		std::map<std::wstring, KeyValues> data{};
	public:
		auto operator[](const wchar_t *key) { return key == nullptr ? KeyValues{} : data[key]; }
	};

	TinyIni(Sections _sections) : sections(_sections) {}

	enum Encoding
	{
		DEFAULT,
		UTF8,
		UTF16LE,
		UTF16BE,
		UTF32LE,
		UTF32BE,
		UTF1,
		UTFEBCDIC,
		SCSU,
		BOCU1,
		GB18030
	};

	Sections sections;
	Encoding encoding = DEFAULT;

	template<typename charType>
	Encoding ConsumeBOM(std::basic_ifstream<charType>& stream)
	{
		unsigned char BOM[4]{ 0 };
		int i = 0;
		while (stream && (i < sizeof(BOM)))
		{
			BOM[i++] = stream.peek();
			stream.get();
			if (BOM[0] == 0xFF && BOM[1] == 0xFE) return UTF16LE;
			if (BOM[0] == 0xFE && BOM[1] == 0xFF) return UTF16BE;
			if (BOM[0] == 0xEF && BOM[1] == 0xBB && BOM[2] == 0xBF) return UTF8;
		}
		stream.seekg(0, std::ios::beg);
		return DEFAULT;
	}

	std::wstring& convert(std::wstring& s, Encoding encoding)
	{
		std::wstring cpy(L"");

		switch (encoding)
		{
		case UTF16LE: {
			for (size_t i{ 0 }; i < s.size(); i += 2)
				cpy += s[i] | (s[i + 1] << 8);
			break;
		}
		case UTF16BE: {
			for (size_t i{ 0 }; i < s.size(); i += 2)
				cpy += s[i + 1] | (s[i] << 8);
			break;
		}
		case UTF8: return s;
		}

		s = cpy;
		return s;
	}

	std::wstring& TrimString(std::wstring& s)
	{
		s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), iswspace));
		s.erase(std::find_if_not(s.rbegin(), s.rend(), iswspace).base(), s.end());
		return s;
	}

	std::pair<std::wstring, std::wstring> SplitKeyValues(std::wstring& line)
	{
		std::wstring key, val;
		TrimString(key = line.substr(0, line.find(L'=')));
		TrimString(val = line.substr(line.find(L'=') + 1, line.size()));
		return  std::pair<std::wstring, std::wstring>{ key, val };
	}

	bool readStream(std::wistream& stream, std::wstring& out, Encoding encoding)
	{
		switch (encoding)
		{
		case DEFAULT:	std::getline(stream, out);
		case UTF8:		std::getline(stream, out);
		case UTF16BE:
		{
			out.clear();
			wchar_t c = 0;
			while (stream && c != L'\n')
			{
				c = (stream.get() << 8) | stream.get();
				out += c;
			}
		}
		case UTF16LE:
		{
			out.clear();
			wchar_t c = 0;
			while (stream && c != L'\n')
			{
				c = stream.get() | (stream.get() << 8);
				out += c;
			}
		}
		}
		return stream.good() || out.size() > 0;
	}

public:
	TinyIni(const char *path, std::locale locale = std::locale(""))
	{
		std::wstring currentSection;
		filestream.open(path, std::ios::binary);
		if (filestream)
		{
			std::ios_base::sync_with_stdio(false);
			encoding = ConsumeBOM(filestream);
			if (encoding == UTF8) filestream.imbue(std::locale("en_US.UTF-8"));
			std::wstring line;
			while (readStream(filestream, line, encoding))
			{
				if (line.empty() || std::all_of(line.begin(), line.end(), [](wchar_t c) { return iswspace(c) || c == 0; }))
					continue;
				if (line.front() == 0) line.erase(line.begin());
				//if (encoding == UTF8) convert(line, UTF8);
				TrimString(line);
				if (line.front() == L';' || line.front() == '#') continue;
				if (line.front() == L'[')
				{
					if (line.back() != L']') continue;
					line.erase(line.begin());
					line.erase(line.end() - 1);
					TrimString(line);
					currentSection = line;
					sections.data.try_emplace(line, KeyValues{});
					continue;
				}
				if (!currentSection.empty())
				{
					if (std::find(line.begin(), line.end(), L'=') == line.end()) continue;
					auto kv = SplitKeyValues(line);
					sections.data[currentSection].data.try_emplace(kv.first, kv.second);
				}
			}
		}
	}

	~TinyIni() noexcept
	{
		if (filestream) filestream.close();
	}

	KeyValues get(const wchar_t *section)
	{
		return (section == nullptr ? KeyValues{} : sections.data[section]);
	}

	std::wstring get(const wchar_t *section, const wchar_t *key)
	{
		return (key == nullptr ? L"" : get(section)[key]);
	}

	KeyValues operator[](const wchar_t *index)
	{
		return (index == nullptr ? KeyValues{} : sections[index]);
	}

	bool exists(const wchar_t *index)
	{
		return sections.data.find(index) != sections.data.end();
	}

	bool exists(const wchar_t *index, const wchar_t *key)
	{
		return exists(index) && sections[index].data.find(key) != sections[index].data.end();
	}

	std::vector<std::wstring> getSectionNames()
	{
		std::vector<std::wstring> names;
		names.reserve(sections.data.size());
		for (const auto& name : sections.data) names.push_back(name.first);
		return names;
	}

	std::vector<std::wstring> getSectionKeys(const wchar_t *section)
	{
		std::vector<std::wstring> keys{};
		keys.reserve(sections[section].data.size());
		for (const auto& key : sections[section].data) keys.push_back(key.first);
		return keys;
	}
};

#endif
