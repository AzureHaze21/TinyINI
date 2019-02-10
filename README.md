# TinyINI
single class INI reader

# Usage

```cpp
#include "TinyINI.h"

TinyIni ini("C:\\path_to_file.ini");
auto section1 = ini[L"Section1"];
auto section2 = ini.get(L"Section1");
auto value1 = ini[L"Section1"][L"Key1"];
auto value2 = ini.get(L"Section2", "Key1");
```
