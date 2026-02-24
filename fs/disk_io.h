#pragma once
#include "types/types.h"

namespace fs {

class DiskIO {
public:
    static constexpr uint32_t SECTOR_SIZE = 512;
    static constexpr uint32_t LBA28_MAX_SECTORS = 0x0FFFFFFF;

    DiskIO() = default;

    bool read_sector(uint32_t lba, void* buffer);
    bool write_sector(uint32_t lba, const void* buffer);
    uint64_t detect_disk_size();

    bool read(uint64_t offset_bytes, void* buffer, uint32_t bytes);
    bool write(uint64_t offset_bytes, const void* buffer, uint32_t bytes);

    void set_disk_size(uint64_t bytes) { disk_size_ = bytes; }
    uint64_t disk_size() const { return disk_size_; }

private:
    static constexpr uint16_t DATA_PORT = 0x1F0;
    static constexpr uint16_t ERROR_PORT = 0x1F1;
    static constexpr uint16_t SECTOR_COUNT_PORT = 0x1F2;
    static constexpr uint16_t LBA_LOW_PORT = 0x1F3;
    static constexpr uint16_t LBA_MID_PORT = 0x1F4;
    static constexpr uint16_t LBA_HIGH_PORT = 0x1F5;
    static constexpr uint16_t DEVICE_PORT = 0x1F6;
    static constexpr uint16_t COMMAND_PORT = 0x1F7;
    static constexpr uint16_t STATUS_PORT = 0x1F7;

    static constexpr uint8_t STATUS_BSY = 0x80;
    static constexpr uint8_t STATUS_DRQ = 0x08;
    static constexpr uint8_t STATUS_ERR = 0x01;

    static constexpr uint8_t CMD_READ = 0x20;
    static constexpr uint8_t CMD_WRITE = 0x30;
    static constexpr uint8_t CMD_IDENTIFY = 0xEC;
    static constexpr uint8_t CMD_FLUSH = 0xE7;

    bool wait_ready(bool for_data) const;
    bool select_drive(uint32_t lba) const;
    bool identify(uint16_t* buffer);
    bool flush();

    uint64_t disk_size_ = 0;
};

}
