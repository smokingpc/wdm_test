// DumpPhysicalDiskData.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <string>

#define SIZE_1MB    1048576

void Usage()
{
    printf("Usage: DumpPhysicalDiskData.exe [physical_drive_devpath] [dump_size_in_MB] [output_file_fullpath]\n");
    printf("e.g.: If you want to dump first 3MB of physical disk 1 to c:\\test\\dumpdata.bin ,\n");
    printf("      you should use command: DumpPhysicalDiskData.exe \\\\.\\PhysicalDrive1 3 c:\\test\\dumpdata.bin \n\n");
}

void DumpDataFromPhysicalDisk(char *disk_devpath, int size_in_mb, char *output_file)
{
    HANDLE disk = CreateFileA(disk_devpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE file = CreateFileA(output_file, GENERIC_WRITE, FILE_SHARE_READ , NULL, CREATE_ALWAYS, 0, NULL);
    int counter = 0;
    if(disk != INVALID_HANDLE_VALUE)
    {
        if(file != INVALID_HANDLE_VALUE)
        {
            UCHAR *buffer = new UCHAR[SIZE_1MB];
            for(int i=0; i<size_in_mb; i++)
            {
                DWORD ret_size = 0;
                DWORD written_size = 0;
                memset(buffer, 0, SIZE_1MB);
                BOOL ok = ReadFile(disk, buffer, SIZE_1MB, &ret_size, NULL);
                if(!ok || 0 == ret_size)
                {
                    printf("Read disk failed, LastError=%d\n", GetLastError());
                    break;
                }
                ok = WriteFile(file, buffer, ret_size, &written_size, NULL);
                if(!ok)
                {
                    printf("Write to file failed, LastError=%d\n", GetLastError());
                    break;
                }

                counter++;
            }
            delete[] buffer;

            CloseHandle(file);
        }
        else
            printf("Open File(%s) FAILED!!\n", output_file);
        CloseHandle(disk);
    }
    else
        printf("Open Disk(%s) FAILED!!\n", disk_devpath);

    printf("Total Read/Write data size = %d MB\n", counter);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        Usage();
        return -1;
    }

    //physical disk symbolic link name, e.g. "\\.\PhysicalDrive4"
    std::string devpath = argv[1];
    int size_mb = atoi(argv[2]);
    std::string filepath = argv[3];

    DumpDataFromPhysicalDisk((char*)devpath.c_str(), size_mb, (char*)filepath.c_str());
}

