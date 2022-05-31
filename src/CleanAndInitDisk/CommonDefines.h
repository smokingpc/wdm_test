#pragma once
#define SIZE_1MB    1048576

typedef std::basic_string<TCHAR> tstring;

typedef struct _CMD_OPTION
{
    tstring DiskDevName = _T("");
    PARTITION_STYLE DiskStyle = PARTITION_STYLE_RAW;
}CMD_OPTION, *PCMD_OPTION;