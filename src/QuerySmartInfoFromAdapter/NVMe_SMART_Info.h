#pragma once

#pragma pack(1)
//refer to NVMe spec 1.3 Figure 93.
//all temperature use Kelvin as unit. not Celsius
//all value data in array are "LITTLE ENDIAN" !!
typedef struct _NVMe_SMART_INFO{
    union 
    {
        struct _CRITICAL_WARN {
            UCHAR AvailableSpaceLow : 1;    // If set to 1, then the available spare space has fallen below the threshold.
            UCHAR TemperatureExceed : 1;    // If set to 1, then a temperature is above an over temperature threshold or below an under temperature threshold.
            UCHAR ReliabilityError:1;   // If set to 1, then the device reliability has been degraded due to significant media related  errors or any internal error that degrades device reliability.
            UCHAR ReadOnlyMode:1;       // If set to 1, then the media has been placed in read only mode
            UCHAR VolatileBackupFailed:1;// If set to 1, then the volatile memory backup device has failed. This field is only valid if the controller has a volatile memory backup solution.
            UCHAR Reserved:3;
        } Warn;
        UCHAR AsByte;
    }CriticalWarn;

    USHORT OverallTemperature;      //Composite Temperature
    UCHAR AvailableSpare;           //Contains a normalized percentage (0 to 100%) of the remaining spare capacity available
    UCHAR AvailableSpareThreshold;  //low limit of available spare in percentage (%)
    UCHAR PercentageUsed;           //life used(in %) of NVME subsystem. refer to JEDEC JESD218A. allow to exceed 100
    UCHAR Reserved[26];
    UCHAR DataUnitRead[16];         //how many 512Bytes blocks the host has read from the controller; if LBA block is 4K, should convert to 512Byte counting(one 4K unit == eight 512byte unit). This value is reported in thousands (i.e., a value of 1 corresponds to 1000 units read) and is rounded up.
    UCHAR DataUnitWrite[16];        //how many 512Bytes blocks the host has write from the controller; if LBA block is 4K, should convert to 512Byte counting(one 4K unit == eight 512byte unit). This value is reported in thousands (i.e., a value of 1 corresponds to 1000 units read) and is rounded up.
    UCHAR HostReadCommands[16];     //Contains the number of read commands completed by the controller. For the NVM command set, this is the number of Compare and Read commands.
    UCHAR HostWrittenCommands[16];  //Contains the number of write commands  completed by  the controller. For the NVM command set, this is the number of Write commands.
    UCHAR ControllerBusyTime[16];   //Contains the amount of time the controller is busy with I/O commands. The controller is busy when there is a command outstanding to an I/O Queue (specifically, a command was issued via an I/O Submission Queue Tail doorbell write and the corresponding  completion queue entry  has not been posted yet to the associated I/O Completion Queue). This value is reported in minutes.
    UCHAR PowerCycle[16];           // Power Cycles: Contains the number of power cycles.
    UCHAR PowerOnHours[16];         // Power On Hours: Contains the number of power-on hours. This does not include time that the controller was powered and in a low power state condition.
    UCHAR UnsafeShutdowns[16];      // Unsafe Shutdowns: Contains the number of unsafe shutdowns. This count is incremented when a shutdown notification (CC.SHN) is not received prior to loss of power.
    UCHAR MediaErrors[16];          // Media Errors:  Contains the number of occurrences where the controller detected an unrecovered data integrity error. Errors such as uncorrectable ECC, CRC checksum failure, or LBA tag mismatch are included in this field.
    UCHAR ErrorInfoLogEntryCount[16];   // Number of Error Information Log Entries:  Contains the number of Error Information log entries over the life of the controller
    ULONG WarningCompositeTemperatureTime;  // Warning Composite Temperature Time: Contains the amount of time in minutes that the controller is operational and the Composite Temperature is greater than or equal to the Warning Composite Temperature Threshold (WCTEMP) field and less than the Critical Composite Temperature Threshold (CCTEMP) field in the Identify Controller data structure
    ULONG CriticalCompositeTemperatureTime; // Critical Composite Temperature Time: Contains the amount of time in minutes that the controller is operational and the Composite Temperature is greater the Critical Composite Temperature Threshold (CCTEMP) field in the Identify Controller data structure
    USHORT  TemperatureSensor1;          // Contains the current temperature reported by temperature sensor 1.
    USHORT  TemperatureSensor2;          // Contains the current temperature reported by temperature sensor 2.
    USHORT  TemperatureSensor3;          // Contains the current temperature reported by temperature sensor 3.
    USHORT  TemperatureSensor4;          // Contains the current temperature reported by temperature sensor 4.
    USHORT  TemperatureSensor5;          // Contains the current temperature reported by temperature sensor 5.
    USHORT  TemperatureSensor6;          // Contains the current temperature reported by temperature sensor 6.
    USHORT  TemperatureSensor7;          // Contains the current temperature reported by temperature sensor 7.
    USHORT  TemperatureSensor8;          // Contains the current temperature reported by temperature sensor 8.
    UCHAR   Reserved1[296];
}NVME_SMART_INFO, *PNVME_SMART_INFO;

//
//typedef struct _smart_health_info_log
//{
//    UINT8  CriticalWarning;            /* Bytes:       0 */
//    UINT16 Temperature;                /* Bytes:     1:2 */
//    UINT8  AvailableSpare;             /* Bytes:       3 */
//    UINT8  AvailableSpareThreshold;    /* Bytes:       4 */
//    UINT8  PercentageUsed;             /* Bytes:       5 */
//    UINT8  Reserved1[26];              /* Bytes:    6:31 */
//    UINT8  DataUnitsRead[16];          /* Bytes:   32:47 */
//    UINT8  DataUnitsWritten[16];       /* Bytes:   48:63 */
//    UINT8  HostReadCommands[16];       /* Bytes:   64:79 */
//    UINT8  HostWriteCommands[16];      /* Bytes:   80:95 */
//    UINT8  ControllerBusyTime[16];     /* Bytes:  96:111 */
//    UINT8  PowerCycles[16];            /* Bytes: 112:127 */
//    UINT8  PowerOnHours[16];           /* Bytes: 128:143 */
//    UINT8  UnsafeShutdowns[16];        /* Bytes: 144:159 */
//    UINT8  MediaErrors[16];            /* Bytes: 160:175 */
//    UINT8  NumErrorInfoLogEntries[16]; /* Bytes: 176:191 */
//    UINT8  Reserved2[320];             /* Bytes: 192:512 */
//} SMART_HEALTH_INFO_LOG, * PSMART_HEALTH_INFO_LOG;
//
//typedef struct _NVM_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY
//{
//    /*
//     * This field indicates critical warnings for the state of the controller.
//     * Each bit corresponds to a critical warning type; multiple bits may be
//     * set. If a bit is cleared to ?? then that critical warning does not
//     * apply. Critical warnings may result in an asynchronous event notification
//     * to the host.
//     */
//    struct
//    {
//
//        /*
//         * If set to ?? then the available spare space has fallen below the
//         * threshold.
//         */
//        UCHAR   AvailableSpaceBelowThreshold : 1;
//
//        /*
//         * If set to ?? then the temperature has exceeded a critical
//         * threshold.
//         */
//        UCHAR   TemperatureExceededCriticalThreshold : 1;
//
//        /*
//         * If set to ?? then the device reliability has been degraded due to
//         * significant media related errors or any internal error that degrades
//         * device reliability.
//         */
//        UCHAR   DeviceReliablityDegraded : 1;
//
//        /* If set to ?? then the media has been placed in read only mode. */
//        UCHAR   MediaInReadOnlyMode : 1;
//
//        /*
//         * If set to ?? then the volatile memory backup device has failed.
//         * This field is only valid if the controller has a volatile memory
//         * backup solution.
//         */
//        UCHAR   VolatileMemoryBackupDeviceFailed : 1;
//        UCHAR   Reserved : 3;
//    } CriticalWarning;
//
//    /*
//     * Contains the temperature of the overall device (controller and NVM
//     * included) in units of Kelvin. If the temperature exceeds the temperature
//     * threshold, refer to section 5.12.1.4, then an asynchronous event may be
//     * issued to the host.
//     */
//    USHORT      Temperature;
//
//    /*
//     * Contains a normalized percentage (0 to 100%) of the remaining spare
//     * capacity available.
//     */
//    UCHAR       AvailableSpare;
//
//    /*
//     * When the Available Spare falls below the threshold indicated in this
//     * field, an asynchronous event may be issued to the host. The value is
//     * indicated as a normalized percentage (0 to 100%).
//     */
//    UCHAR       AvailableSpareThreshold;
//
//    /*
//     * Contains a vendor specific estimate of the percentage of device life used
//     * based on the actual device usage and the manufacturer’s prediction of
//     * device life. A value of 100 indicates that the estimated endurance of the
//     * device has been consumed, but may not indicate a device failure. The
//     * value is allowed to exceed 100. Percentages greater than 254 shall be
//     * represented as 255. This value shall be updated once per power-on hour
//     * (when the controller is not in a sleep state). Refer to the JEDEC JESD218
//     * standard for SSD device life and endurance measurement techniques.
//     */
//    UCHAR       PercentageUsed;
//    UCHAR       Reserved1[26];
//
//    /*
//     * Contains the number of 512 byte data units the host has read from the
//     * controller; this value does not include metadata. This value is reported
//     * in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes
//     * read) and is rounded up. When the LBA size is a value other than 512
//     * bytes, the controller shall convert the amount of data read to 512 byte
//     * units. For the NVM command set, logical
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } DataUnitsRead;
//
//    /*
//     * Contains the number of 512 byte data units the host has written to the
//     * controller; this value does not include metadata. This value is reported
//     * in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes
//     * written) and is rounded up. When the LBA size is a value other than 512
//     * bytes, the controller shall convert the amount of data written to 512
//     * byte units. For the NVM command set, logical blocks written as part of
//     * Write operations shall be included in this value. Write Uncorrectable
//     * commands shall not impact this value.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } DataUnitsWritten;
//
//    /*
//     * Contains the number of read commands issued to the controller. For the
//     * NVM command set, this is the number of Compare and Read commands.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } HostReadCommands;
//
//    /*
//     * Contains the number of write commands issued to the controller. For the
//     * NVM command set, this is the number of Write commands.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } HostWriteCommands;
//
//    /*
//     * Contains the amount of time the controller is busy with I/O commands. The
//     * controller is busy when there is a command outstanding to an I/O Queue
//     * (specifically, a command was issued via an I/O Submission Queue Tail
//     * doorbell write and the corresponding completion entry has not been posted
//     * yet to the associated I/O Completion Queue). This value is reported in
//     * minutes.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } ControllerBusyTime;
//
//    /* Contains the number of power cycles. */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } PowerCycles;
//
//    /*
//     * Contains the number of power-on hours. This does not include time that
//     * the controller was powered and in a low power state condition.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } PowerOnHours;
//
//    /*
//     * Contains the number of unsafe shutdowns. This count is incremented when a
//     * shutdown notification (CC.SHN) is not received prior to loss of power.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } UnsafeShutdowns;
//
//    /*
//     * Contains the number of occurrences where the controller detected an
//     * unrecovered data integrity error. Errors such as uncorrectable ECC, CRC
//     * checksum failure, or LBA tag mismatch are included in this field.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } MediaErrors;
//
//    /*
//     * Contains the number of Error Information log entries over the life of the
//     * controller.
//     */
//    struct
//    {
//        ULONGLONG Lower;
//        ULONGLONG Upper;
//    } NumberofErrorInformationLogEntries;
//
//    UCHAR Reserved2[320];
//} ADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY,
//* PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY;
//
#pragma pack()
