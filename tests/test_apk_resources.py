import pathlib
import struct
import sys
import unittest
import zipfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from apk_resources import ORIGINAL, TARGET, packages, patch_resources

class ResourcePatchTests(unittest.TestCase):
    def fixture(self):
        package = struct.pack('<HHII', 0x200, 284, 288, 0x7f)
        package += ORIGINAL.encode('utf-16le').ljust(256, b'\0')
        package += b'\0' * 16 + b'TEST'
        return struct.pack('<HHII', 2, 12, 12 + len(package), 1) + package

    def test_preserves_ids_and_all_non_name_bytes(self):
        original = self.fixture()
        patched = patch_resources(original)
        self.assertEqual(packages(patched), [(12, 0x7f, TARGET)])
        self.assertEqual(original[:24], patched[:24])
        self.assertEqual(original[280:], patched[280:])
        self.assertEqual(len(original), len(patched))

    def test_rejects_wrong_package_and_double_patch(self):
        with self.assertRaises(ValueError):
            patch_resources(patch_resources(self.fixture()))

    def test_rejects_corrupt_sizes(self):
        data = self.fixture()
        for truncated in [data[:5], data[:-1], data[:16]]:
            with self.assertRaises(ValueError):
                patch_resources(truncated)
        data = bytearray(data)
        struct.pack_into('<I', data, 16, 0)
        with self.assertRaises(ValueError):
            patch_resources(data)

if __name__ == '__main__':
    unittest.main()
