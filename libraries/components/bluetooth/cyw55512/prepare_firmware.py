"""Prepare the CYW55500A1 controller patch for the Edgi-Talk CYW55512."""

import argparse
import hashlib
from pathlib import Path
import tempfile
from urllib.request import urlopen


REVISION = 'e5f0a31b95d51bbeb7ea5ef152ab0ff563961ff8'
FILENAME = 'CYW55500A1_001.002.032.0253.0000_Generic_UART_37_4MHz_wlbga_iPA_sLNA_ANT0.hcd'
GENERIC_URL = f'https://raw.githubusercontent.com/Infineon/ifx-linux-bluetooth/{REVISION}/fw/{FILENAME}'
GENERIC_SHA256 = '02609e8ebcf4e1d84d8250e9f5e1dd1d205b89d0f2f78a16a5c3078a2214cee2'


def validate_hcd(image):
    offset = 0
    writes = 0
    while offset + 3 <= len(image):
        opcode = int.from_bytes(image[offset:offset + 2], 'little')
        size = image[offset + 2]
        offset += 3 + size
        if offset > len(image):
            break
        if opcode == 0xfc4e and size == 4 and writes and offset == len(image):
            return
        if opcode != 0xfc4c or size <= 4:
            break
        writes += 1
    raise ValueError('Expected complete CYW55500A1 Write RAM records and a final Launch RAM')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--hcd', type=Path,
                        help='use a local HCD for the installed module instead of the Infineon generic image')
    args = parser.parse_args()
    try:
        if args.hcd:
            image = args.hcd.read_bytes()
        else:
            with urlopen(GENERIC_URL, timeout=60) as response:
                image = response.read()
            if hashlib.sha256(image).hexdigest() != GENERIC_SHA256:
                parser.error('Infineon firmware SHA-256 mismatch')
        validate_hcd(image)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    output = Path(__file__).with_name('cyw55512_hcd.inc')
    with tempfile.NamedTemporaryFile(mode='w', encoding='ascii', newline='\n',
                                     dir=output.parent, delete=False) as temp:
        for start in range(0, len(image), 16):
            temp.write(','.join(f'0x{byte:02x}' for byte in image[start:start + 16]) + ',\n')
    Path(temp.name).replace(output)
    print(f'Prepared {len(image)} bytes in {output} (SHA-256 {hashlib.sha256(image).hexdigest()})')


if __name__ == '__main__':
    main()
