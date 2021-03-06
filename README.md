MMF2/CF2.5 Open Source Extension Repository
===
A single repository to contain and/or link to open source extensions for MMF2/CF2.5.
These extensions are collated for examples and for upgrading; these are subject to individual licenses.
If you plan on distributing your own version publicly, it is highly recommended you get permission from original authors, where possible.
Since these source codes are collated by Phi, not by the authors themselves, they may be old versions, but should be suitable for demonstration.

### Using a single project ###
For exporting a single project, you will need to download the project folder, the Lib, and the Inc folder.
For example, DarkEdif Template can be extracted by downloading DarkEdif\DarkEdif Template, DarkEdif\Inc, and DarkEdif\Lib.
All projects in this repository use a shared Visual Studio props file, found in Lib, which will set all general vcxproj settings, automatically checking what SDK is in use. This has several effects:

1. You won't find most project settings entered in the vcxproj; you should instead open the Edit dialog for a property and view the "evaluated version".
2. If you are converting from one SDK to another, most of the work will be done for you.
3. Adding a new project configuration (example, you have Edittime, and you add Edittime Unicode), the props file will read the project configuration name and apply the settings for Edittime and Unicode; making it non-debug, adding the `_UNICODE` defines, etc.

## SDK variants
### MMF2SDK
MMF2SDK is the original MMF2SDK provided by Clickteam, programmed in C, with use of some preprocessor macros. It's a lot of manual work, and IntelliSense doesn't like it much (IntelliSense isn't great with preprocessor macros that start/end functions/sections).

### MMF2SDK_Unicode
MMF2SDK_Unicode is the MMF2SDK provided by Clickteam, with both ANSI (non-Unicode) and Unicode builds possible. Ease of use is about equal with MMF2SDK, as long as you understand [how to convert an ANSI project to ANSI & Unicode](#markdown-anchor-how-to-convert-ansi-functions-to-ansi--unicode).

### rSDK
rSDK is a change up with even heavier use of preprocessor macros, written by James with contribution from other users. [CT forum thread](https://community.clickteam.com/threads/42183-rSDK).  It was introduced with extra macros that did away quite a bit of work of MMF2SDK, but still a lot of manual work and IntelliSense doesn't like it either.

### Edif
Edif is a newer, C++ SDK that handles most of the messy parts of Fusion, and does away with the heavy use of preprocessor macros inherent to older SDKs. This was written by James as well, some time later. [CT forum thread](https://community.clickteam.com/threads/61692-Edif-Extension-Development-Is-Fun).

### DarkEdif
DarkEdif is a continuation of Edif with changes mostly coded by Phi, with some code contributions from LB and other developers. [CT forum thread](https://community.clickteam.com/threads/71793-DarkEDIF-Taking-suggestions?p=608099&viewfull=1#post608099).

It includes all the features of Edif and some extra, including:

* Multi-language JSON file
* Properties defined in the JSON file
* In debug build, a runtime check that A/C/E parameters in C++ and JSON are the same
  (only active in Debug/Debug Unicode builds; disable via defining `FAST_ACE_LINK` in project properties)
* A JSON minifier for runtime builds
* Error messages during Edif crash scenarios (e.g. missing actions)
* Extension description defined at runtime via `JSON_COMMENT_MACRO` (demo: Bluewing Client/Server, Common.h)
* Fusion debugger access via `DarkEdif::FusionDebugger` (demo: Bluewing Client/Server, Extension.cpp, Extension.h)
* An opt-in SDK and extension update checker tool

DarkEdif uses a pre-build standalone tool (and in non-Windows platforms, also a post-build standalone tool) programmed in C#. This tool is currently not open-source.

More details on DarkEdif are available in the Wiki, see:
* [DarkEdif ext dev features](https://github.com/SortaCore/MMF2Exts/wiki/DarkEdif-ext-dev-features) for a list of features available to a DarkEdif extension developer
* [DarkEdif Fusion user features](https://github.com/SortaCore/MMF2Exts/wiki/DarkEdif-Fusion-user-features) for a list of features available to Fusion users

## How to convert ANSI functions to ANSI & Unicode
Make sure you're aware of what any text-related function you call expects. Does it ask for number of elements in array, or number of bytes in array?
Note TCHAR is a preprocessor definition provided by Microsoft, which will be replaced with char in ANSI builds, and wchar_t in Unicode builds.

If you don't want to provide Fusion 2.0 ANSI compatiblity, you can remove the non-Unicode project configurations from your project. Both CF2.5 and Fusion 2.0 Unicode can still use ANSI-only extensions, but both use Unicode by default if both variants of an extension are available.

* `char` -> `TCHAR`
* `strcpy()` -> `_tcscpy()`; any `strXX()` becomes `_tcsXX()`, for things like `sprintf()` you may have to google to make sure you're using the right one. If in doubt, MSDN normally provides the TCHAR equivalent.
* `sizeof(char[] variable)` -> `sizeof(TCHAR[] variable) / sizeof(TCHAR)` (for functions expecting array sizes in elements)
* `sizeof(char[] variable)` -> `sizeof(TCHAR[] variable) * sizeof(TCHAR)` (for functions expecting array sizes in bytes)
* `std::string` -> `std::tstring`, likewise for `std::stringstream` and `std::string_view`; a feature of DarkEdif, evil as it's not part of std, but a readability feature.
