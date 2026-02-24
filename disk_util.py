#!/usr/bin/env python3
"""
Utility to manage the OS disk.img filesystem.

Disk layout:
- Superblock at offset 0 (512 bytes)
- File headers (linked list, 80 bytes each)
- File data allocated from free_space

Usage:
    python disk_util.py list disk.img
    python disk_util.py add disk.img <local_file> [<dest_name>]
    python disk_util.py extract disk.img <file_name> <local_file>
    python disk_util.py format disk.img [<size_bytes>]
    python disk_util.py dump disk.img
"""

import argparse
import struct
import os
import sys
from dataclasses import dataclass
from typing import Optional, List, Tuple

FS_MAGIC = 0x4C465331
MAX_NAME_LEN = 64
SUPERBLOCK_SIZE = 512
FILE_HEADER_SIZE = 88  # 64 (name) + 4 (size) + 8 (next) + 8 (data_offset) + 1 (in_use) + 3 (reserved)


@dataclass
class Superblock:
    magic: int
    block_size: int
    first_file: int
    free_space: int
    disk_size: int

    @classmethod
    def from_bytes(cls, data: bytes) -> 'Superblock':
        if len(data) < SUPERBLOCK_SIZE:
            raise ValueError(f"Superblock data too short: {len(data)}")
        magic, block_size = struct.unpack_from('<II', data, 0)
        first_file, free_space, disk_size = struct.unpack_from('<QQQ', data, 8)
        return cls(magic, block_size, first_file, free_space, disk_size)

    def to_bytes(self) -> bytes:
        data = bytearray(SUPERBLOCK_SIZE)
        struct.pack_into('<II', data, 0, self.magic, self.block_size)
        struct.pack_into('<QQQ', data, 8, self.first_file, self.free_space, self.disk_size)
        return bytes(data)


@dataclass
class FileHeader:
    name: str
    size: int
    next_header: int
    data_offset: int
    in_use: bool

    @classmethod
    def from_bytes(cls, data: bytes) -> 'FileHeader':
        name_bytes = data[:MAX_NAME_LEN]
        name = name_bytes.rstrip(b'\x00').decode('utf-8', errors='replace')
        size, = struct.unpack_from('<I', data, 64)
        next_header, data_offset = struct.unpack_from('<QQ', data, 68)
        in_use = data[84] != 0
        return cls(name, size, next_header, data_offset, in_use)

    def to_bytes(self) -> bytes:
        data = bytearray(FILE_HEADER_SIZE)
        name_bytes = self.name.encode('utf-8')[:MAX_NAME_LEN - 1]
        data[:len(name_bytes)] = name_bytes
        struct.pack_into('<I', data, 64, self.size)
        struct.pack_into('<QQ', data, 68, self.next_header, self.data_offset)
        data[84] = 1 if self.in_use else 0
        return bytes(data)


class DiskImage:
    def __init__(self, path: str):
        self.path = path
        self.data = bytearray()
        self.superblock: Optional[Superblock] = None
        self._load()

    def _load(self):
        if os.path.exists(self.path):
            with open(self.path, 'rb') as f:
                self.data = bytearray(f.read())
            if len(self.data) >= SUPERBLOCK_SIZE:
                self.superblock = Superblock.from_bytes(self.data)
        else:
            self.data = bytearray()

    def _save(self):
        with open(self.path, 'wb') as f:
            f.write(self.data)

    def _ensure_size(self, size: int):
        if len(self.data) < size:
            self.data.extend(b'\x00' * (size - len(self.data)))

    def _read_at(self, offset: int, size: int) -> bytes:
        self._ensure_size(offset + size)
        return bytes(self.data[offset:offset + size])

    def _write_at(self, offset: int, data: bytes):
        self._ensure_size(offset + len(data))
        self.data[offset:offset + len(data)] = data

    def _allocate(self, size: int) -> int:
        offset = self.superblock.free_space
        self.superblock.free_space += size
        self._write_at(0, self.superblock.to_bytes())
        return offset

    def is_formatted(self) -> bool:
        return self.superblock is not None and self.superblock.magic == FS_MAGIC

    def format(self, size: int = 1024 * 1024):
        self.superblock = Superblock(
            magic=FS_MAGIC,
            block_size=512,
            first_file=0,
            free_space=SUPERBLOCK_SIZE,
            disk_size=size
        )
        self._ensure_size(size)
        self._write_at(0, self.superblock.to_bytes())
        self._save()

    def list_files(self) -> List[Tuple[str, int]]:
        if not self.is_formatted():
            return []

        files = []
        current = self.superblock.first_file
        while current != 0:
            header_data = self._read_at(current, FILE_HEADER_SIZE)
            header = FileHeader.from_bytes(header_data)
            if header.in_use:
                files.append((header.name, header.size))
            current = header.next_header
        return files

    def find_file(self, name: str) -> Tuple[Optional[int], Optional[FileHeader]]:
        current = self.superblock.first_file
        while current != 0:
            header_data = self._read_at(current, FILE_HEADER_SIZE)
            header = FileHeader.from_bytes(header_data)
            if header.in_use and header.name == name:
                return current, header
            current = header.next_header
        return None, None

    def add_file(self, local_path: str, dest_name: Optional[str] = None):
        if not self.is_formatted():
            print("Error: Disk not formatted. Run 'format' first.")
            return False

        if dest_name is None:
            dest_name = os.path.basename(local_path)

        if len(dest_name) >= MAX_NAME_LEN:
            print(f"Error: Name too long (max {MAX_NAME_LEN - 1} chars)")
            return False

        with open(local_path, 'rb') as f:
            content = f.read()

        existing_offset, existing_header = self.find_file(dest_name)
        if existing_header is not None:
            print(f"Overwriting existing file: {dest_name}")
            data_offset = self._allocate(len(content))
            self._write_at(data_offset, content)
            existing_header.size = len(content)
            existing_header.data_offset = data_offset
            self._write_at(existing_offset, existing_header.to_bytes())
        else:
            header_offset = self._allocate(FILE_HEADER_SIZE)
            data_offset = self._allocate(len(content))

            self._write_at(data_offset, content)

            header = FileHeader(
                name=dest_name,
                size=len(content),
                next_header=0,
                data_offset=data_offset,
                in_use=True
            )
            self._write_at(header_offset, header.to_bytes())

            if self.superblock.first_file == 0:
                self.superblock.first_file = header_offset
                self._write_at(0, self.superblock.to_bytes())
            else:
                tail_offset, tail_header = self._find_tail()
                if tail_header:
                    tail_header.next_header = header_offset
                    self._write_at(tail_offset, tail_header.to_bytes())

        self._save()
        print(f"Added: {dest_name} ({len(content)} bytes)")
        return True

    def _find_tail(self) -> Tuple[Optional[int], Optional[FileHeader]]:
        current = self.superblock.first_file
        prev_offset = None
        prev_header = None
        while current != 0:
            header_data = self._read_at(current, FILE_HEADER_SIZE)
            header = FileHeader.from_bytes(header_data)
            prev_offset = current
            prev_header = header
            current = header.next_header
        return prev_offset, prev_header

    def extract_file(self, name: str, local_path: str) -> bool:
        if not self.is_formatted():
            print("Error: Disk not formatted.")
            return False

        offset, header = self.find_file(name)
        if header is None:
            print(f"Error: File not found: {name}")
            return False

        data = self._read_at(header.data_offset, header.size)
        with open(local_path, 'wb') as f:
            f.write(data)

        print(f"Extracted: {name} -> {local_path} ({header.size} bytes)")
        return True

    def dump(self):
        print("=== Disk Image Dump ===")
        if not self.is_formatted():
            print("Not formatted.")
            return

        print(f"Magic:       0x{self.superblock.magic:08X}")
        print(f"Block size:  {self.superblock.block_size}")
        print(f"First file:  {self.superblock.first_file}")
        print(f"Free space:  {self.superblock.free_space}")
        print(f"Disk size:   {self.superblock.disk_size}")
        print(f"Image size:  {len(self.data)}")
        print()
        print("Files:")
        for name, size in self.list_files():
            print(f"  {name}: {size} bytes")


def main():
    parser = argparse.ArgumentParser(
        description='Manage OS disk.img filesystem',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python disk_util.py format disk.img 1048576
  python disk_util.py add disk.img myfile.txt
  python disk_util.py add disk.img myfile.txt renamed.txt
  python disk_util.py list disk.img
  python disk_util.py extract disk.img myfile.txt output.txt
  python disk_util.py dump disk.img
"""
    )
    subparsers = parser.add_subparsers(dest='command', help='Command')

    fmt_parser = subparsers.add_parser('format', help='Format disk image')
    fmt_parser.add_argument('disk', help='Disk image path')
    fmt_parser.add_argument('size', type=int, nargs='?', default=1024*1024, help='Disk size in bytes')

    add_parser = subparsers.add_parser('add', help='Add file to disk')
    add_parser.add_argument('disk', help='Disk image path')
    add_parser.add_argument('file', help='Local file to add')
    add_parser.add_argument('name', nargs='?', help='Destination name')

    extract_parser = subparsers.add_parser('extract', help='Extract file from disk')
    extract_parser.add_argument('disk', help='Disk image path')
    extract_parser.add_argument('name', help='File name in disk')
    extract_parser.add_argument('output', help='Local output path')

    list_parser = subparsers.add_parser('list', help='List files')
    list_parser.add_argument('disk', help='Disk image path')

    dump_parser = subparsers.add_parser('dump', help='Dump disk info')
    dump_parser.add_argument('disk', help='Disk image path')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    disk = DiskImage(args.disk)

    if args.command == 'format':
        disk.format(args.size)
        print(f"Formatted {args.disk} ({args.size} bytes)")
    elif args.command == 'add':
        disk.add_file(args.file, args.name)
    elif args.command == 'extract':
        disk.extract_file(args.name, args.output)
    elif args.command == 'list':
        for name, size in disk.list_files():
            print(f"{name}: {size} bytes")
    elif args.command == 'dump':
        disk.dump()


if __name__ == '__main__':
    main()
