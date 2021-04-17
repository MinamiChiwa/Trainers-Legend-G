using System.Runtime.InteropServices;

namespace TextDumper
{
    public static class HashLib
    {
        /* 
        // Source code of this lib, very simple, just exports std::hash from MSVC
        // Too lazy to write again
        #include <string>

        extern "C" _declspec(dllexport) size_t hash(wchar_t* str)
        {
        	return std::hash<std::wstring>{}(str);
        }
        */

        [DllImport("hash-lib.dll", EntryPoint = "hash", CharSet = CharSet.Unicode)]
        public static extern ulong HashString(string str);
    }
}