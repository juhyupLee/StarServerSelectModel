#pragma once
#define ERROR_LOG(string)   PrintErrorLog(string,__LINE__)
void PrintErrorLog(const wchar_t* str, int line);
