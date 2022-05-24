#pragma once
void QueryDeviceProperties(HANDLE disk);
void QueryAdapterProperties(HANDLE disk);
void QueryWriteCacheProperties(HANDLE disk);
void QueryDiskGeometry(HANDLE disk);
void QueryDiskCapacity(HANDLE disk);