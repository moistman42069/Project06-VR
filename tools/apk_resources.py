"""Rename the pinned resource namespace without changing resource IDs or payloads.

Format: AOSP ResourceTypes.h ResTable_package (name is char16_t[128]).
"""
import struct

ORIGINAL = 'com.NightsofKronos.SonictheHedgehog'
TARGET = 'com.p06.quest'

def packages(data):
    if len(data) < 12:
        raise ValueError('Truncated resource table')
    kind, header, size, count = struct.unpack_from('<HHII', data)
    if kind != 2 or header != 12 or size != len(data):
        raise ValueError('Invalid resource table header')
    offset = header
    found = []
    while offset < size:
        if offset + 8 > size:
            raise ValueError('Truncated chunk')
        kind, head, length = struct.unpack_from('<HHI', data, offset)
        if head < 8 or length < head or offset + length > size:
            raise ValueError('Invalid chunk bounds')
        if kind == 0x200:
            if head < 284:
                raise ValueError('Invalid package header')
            name = data[offset+12:offset+268].decode('utf-16le').split('\0', 1)[0]
            found.append((offset, struct.unpack_from('<I', data, offset+8)[0], name))
        offset += length
    if len(found) != count:
        raise ValueError('Package count mismatch')
    return found

def patch_resources(data):
    found = packages(data)
    if len(found) != 1 or found[0][1:] != (0x7f, ORIGINAL):
        raise ValueError('Unexpected resource namespace; refusing unpinned patch')
    start = found[0][0] + 12
    result = bytearray(data)
    result[start:start+256] = TARGET.encode('utf-16le').ljust(256, b'\0')
    assert result[:start] == data[:start] and result[start+256:] == data[start+256:]
    assert packages(result)[0][1:] == (0x7f, TARGET)
    return bytes(result)
