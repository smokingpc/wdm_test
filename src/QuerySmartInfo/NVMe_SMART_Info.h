#pragma once

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
