// WriteDataToPhysicalDisk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <string>
#define SIZE_1MB    1048576

void Usage()
{
    printf("Usage: WriteDataToPhysicalDisk.exe [src_file_fullpath] [physical_drive_devpath] \n");
    printf("e.g.: If you want load c:\\output\\dumpdata.bin to physical disk 4,\n");
    printf("      you should use command: WriteDataToPhysicalDisk.exe c:\\output\\dumpdata.bin \\\\.\\PhysicalDrive4 \n\n");
}

void WriteDataToPhysicalDisk(char* disk_devpath, char* src_file)
{
    HANDLE disk = CreateFileA(disk_devpath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE file = CreateFileA(src_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (disk != INVALID_HANDLE_VALUE)
    {
        if (file != INVALID_HANDLE_VALUE)
        {
            UCHAR* buffer = new UCHAR[SIZE_1MB];
            memset(buffer, 0, SIZE_1MB);
            DWORD ret_size = 0;

            while(TRUE == ReadFile(file, buffer, SIZE_1MB, &ret_size, NULL) && ret_size > 0)
            {
                DWORD written_size = 0;

                BOOL ok = WriteFile(disk, buffer, ret_size, &written_size, NULL);
                if(!ok || 0 == written_size)
                    break;
                else
                    printf("Wrote data block (%d)Bytes\n", written_size);
                memset(buffer, 0, SIZE_1MB);
            }

            delete[] buffer;

            CloseHandle(file);
        }
        else
            printf("Open File(%s) FAILED!!\n", src_file);
        CloseHandle(disk);
    }
    else
        printf("Open Disk(%s) FAILED!!\n", disk_devpath);
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        Usage();
        return -1;
    }

    //devpath is physical disk symbolic link name, e.g. "\\.\PhysicalDrive4"
    char* filepath = argv[1];
    char* devpath = argv[2];
    WriteDataToPhysicalDisk(devpath, filepath);
}
