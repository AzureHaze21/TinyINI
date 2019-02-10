#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>

class TinyIni
{
public:
	using KeyValues = std::map<std::wstring, std::wstring>;
	using Sections = std::map<std::wstring, KeyValues>;
private:
	Sections sections;

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

	template<typename charType>
	Encoding ConsumeBOM(std::basic_ifstream<charType>& stream)
	{
		unsigned char BOM[4]{ 0 };
		charType c = 0;
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

	std::wstring& utf16le_to_utf8(std::wstring& s)
	{
		std::wstring cpy(L"");
		for (size_t i{ 0 }; i < s.size(); i += 2)
			cpy += s[i] | (s[i + 1] << 8);
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
public:

	TinyIni(const char *path, std::locale locale = std::locale(""))
	{
		std::wstring currentSection;
		if (std::wifstream ifs(path, std::ios::binary); ifs)
		{
			std::ios_base::sync_with_stdio(false);
			Encoding encoding = ConsumeBOM(ifs);
			ifs.imbue(locale);
			std::wstring line;
			while (std::getline(ifs, line))
			{
				if (line.empty() || std::all_of(line.begin(), line.end(), [](wchar_t c) { return iswspace(c) || c == 0; }))
					continue;
				if (line.front() == 0) line.erase(line.begin());
				if (encoding != DEFAULT) utf16le_to_utf8(line);
				TrimString(line);
				if (line.front() == L';' || line.front() == '#') continue;
				if (line.front() == L'[')
				{
					if (line.back() != L']') continue;
					line.erase(line.begin());
					line.erase(line.end() - 1);
					TrimString(line);
					currentSection = line;
					sections.try_emplace(line, KeyValues{});
					continue;
				}
				if (!currentSection.empty())
				{
					if (std::find(line.begin(), line.end(), L'=') == line.end()) continue;
					auto kv = SplitKeyValues(line);
					sections[currentSection].try_emplace(kv.first, kv.second);
				}
			}
		}
	}

	KeyValues get(const wchar_t *section)
	{
		return (section == nullptr ? KeyValues{} : sections[section]);
	}

	std::wstring get(const wchar_t *section, const wchar_t *key)
	{
		return (key == nullptr ? L"" : get(section)[key]);
	}

	KeyValues operator[](const wchar_t *index)
	{
		return (index == nullptr ? KeyValues{} : sections[index]);
	}
};

