#include <fstream>
#include <map>
#include <algorithm>

class TinyIni
{
private:
	static std::wstring& TrimString(std::wstring& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char c) { return c != ' '; }));
		s.erase(std::find_if(s.rbegin(), s.rend(), [](char c) { return c != ' '; }).base(), s.end());
		return s;
	}

	static std::pair<std::wstring, std::wstring> SplitKeyValues(std::wstring& line)
	{
		std::wstring key, val;
		TrimString(key = line.substr(0, line.find('=')));
		TrimString(val = line.substr(line.find('=') + 1, line.size()));
		return  std::pair<std::wstring, std::wstring>{ key, val };
	}
public:
	static auto Read(const char *path)
	{
		using KeyValues = std::map<std::wstring, std::wstring>;
		using Sections = std::map<std::wstring, KeyValues>;
		Sections sections;
		std::wstring currentSection = L"";
		if (std::wifstream ifs(path); ifs)
		{
			std::wstring line;
			while (std::getline(ifs, line))
			{
				if (line.empty()) continue;
				// trim both ends of the string
				TrimString(line);
				if (line.front() == ';') continue;
				if (line.front() == '[')
				{
					if (line.back() != ']') continue;
					line.erase(line.begin());
					line.erase(line.end() - 1);
					TrimString(line);
					currentSection = line;
					sections.try_emplace(line, KeyValues{});
					continue;
				}
				if (!currentSection.empty())
				{
					if (std::find(line.begin(), line.end(), '=') == line.end()) continue;
					auto kv = SplitKeyValues(line);
					sections[currentSection].try_emplace(kv.first, kv.second);
				}
			}
		}
		return sections;
	}
};
