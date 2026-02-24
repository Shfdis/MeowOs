#include "fs/disk_io.h"
#include "types/cpu.h"

namespace fs {

bool DiskIO::wait_ready(bool for_data) const {
    for (int i = 0; i < 1000000; ++i) {
        uint8_t status = inb(STATUS_PORT);
        if (status & STATUS_ERR)
            return false;
        if (!(status & STATUS_BSY)) {
            if (!for_data)
                return true;
            if (status & STATUS_DRQ)
                return true;
        }
    }
    return false;
}

bool DiskIO::select_drive(uint32_t lba) const {
    outb(DEVICE_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(SECTOR_COUNT_PORT, 1);
    outb(LBA_LOW_PORT, lba & 0xFF);
    outb(LBA_MID_PORT, (lba >> 8) & 0xFF);
    outb(LBA_HIGH_PORT, (lba >> 16) & 0xFF);
    return wait_ready(false);
}

bool DiskIO::read_sector(uint32_t lba, void* buffer) {
    if (!select_drive(lba))
        return false;
    outb(COMMAND_PORT, CMD_READ);
    if (!wait_ready(true))
        return false;
    uint16_t* ptr = static_cast<uint16_t*>(buffer);
    for (int i = 0; i < 256; ++i)
        ptr[i] = inw(DATA_PORT);
    return true;
}

bool DiskIO::write_sector(uint32_t lba, const void* buffer) {
    if (!select_drive(lba))
        return false;
    outb(COMMAND_PORT, CMD_WRITE);
    if (!wait_ready(true))
        return false;
    const uint16_t* ptr = static_cast<const uint16_t*>(buffer);
    for (int i = 0; i < 256; ++i)
        outw(DATA_PORT, ptr[i]);
    if (!wait_ready(false))
        return false;
    return flush();
}

bool DiskIO::flush() {
    outb(DEVICE_PORT, 0xE0);
    outb(COMMAND_PORT, CMD_FLUSH);
    return wait_ready(false);
}

bool DiskIO::identify(uint16_t* buffer) {
    outb(DEVICE_PORT, 0xE0);
    outb(SECTOR_COUNT_PORT, 0);
    outb(LBA_LOW_PORT, 0);
    outb(LBA_MID_PORT, 0);
    outb(LBA_HIGH_PORT, 0);
    outb(COMMAND_PORT, CMD_IDENTIFY);
    if (!wait_ready(true))
        return false;
    for (int i = 0; i < 256; ++i)
        buffer[i] = inw(DATA_PORT);
    return true;
}

uint64_t DiskIO::detect_disk_size() {
    uint16_t id[256];
    if (!identify(id))
        return 0;
    uint64_t sectors = 0;
    if (id[83] & 0x0400) {
        sectors = static_cast<uint64_t>(id[100]) |
                  (static_cast<uint64_t>(id[101]) << 16) |
                  (static_cast<uint64_t>(id[102]) << 32) |
                  (static_cast<uint64_t>(id[103]) << 48);
    } else {
        sectors = static_cast<uint64_t>(id[60]) |
                  (static_cast<uint64_t>(id[61]) << 16);
    }
    if (sectors > LBA28_MAX_SECTORS)
        sectors = LBA28_MAX_SECTORS;
    return sectors * SECTOR_SIZE;
}

bool DiskIO::read(uint64_t offset_bytes, void* buffer, uint32_t bytes) {
    if (bytes == 0)
        return true;
    if (offset_bytes + bytes > disk_size_)
        return false;
    uint8_t* dst = static_cast<uint8_t*>(buffer);
    while (bytes > 0) {
        uint32_t lba = static_cast<uint32_t>(offset_bytes / SECTOR_SIZE);
        if (lba > LBA28_MAX_SECTORS)
            return false;
        uint32_t offset_in_sector = offset_bytes % SECTOR_SIZE;
        uint32_t chunk = SECTOR_SIZE - offset_in_sector;
        if (chunk > bytes)
            chunk = bytes;
        if (offset_in_sector == 0 && chunk == SECTOR_SIZE) {
            if (!read_sector(lba, dst))
                return false;
        } else {
            uint8_t sector_buf[SECTOR_SIZE];
            if (!read_sector(lba, sector_buf))
                return false;
            for (uint32_t i = 0; i < chunk; ++i)
                dst[i] = sector_buf[offset_in_sector + i];
        }
        dst += chunk;
        offset_bytes += chunk;
        bytes -= chunk;
    }
    return true;
}

bool DiskIO::write(uint64_t offset_bytes, const void* buffer, uint32_t bytes) {
    if (bytes == 0)
        return true;
    if (offset_bytes + bytes > disk_size_)
        return false;
    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    while (bytes > 0) {
        uint32_t lba = static_cast<uint32_t>(offset_bytes / SECTOR_SIZE);
        if (lba > LBA28_MAX_SECTORS)
            return false;
        uint32_t offset_in_sector = offset_bytes % SECTOR_SIZE;
        uint32_t chunk = SECTOR_SIZE - offset_in_sector;
        if (chunk > bytes)
            chunk = bytes;
        if (offset_in_sector == 0 && chunk == SECTOR_SIZE) {
            if (!write_sector(lba, src))
                return false;
        } else {
            uint8_t sector_buf[SECTOR_SIZE];
            if (!read_sector(lba, sector_buf))
                return false;
            for (uint32_t i = 0; i < chunk; ++i)
                sector_buf[offset_in_sector + i] = src[i];
            if (!write_sector(lba, sector_buf))
                return false;
        }
        src += chunk;
        offset_bytes += chunk;
        bytes -= chunk;
    }
    return true;
}

}
