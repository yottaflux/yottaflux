#!/usr/bin/env python3
# Copyright (c) 2024 The Yottaflux Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Generate valid Base58Check burn addresses for Yottaflux.

Burn addresses are provably unspendable because they are derived from a
human-readable label (e.g. "issueAsset") padded with X characters, rather
than from a real public key hash. No one can produce a private key whose
hash160 matches the contrived payload.

The algorithm produces addresses like:
    YissueAssetXXXXXXXXXXXXXXXXXXaP4rB

where the readable prefix identifies the purpose, the X's are padding,
and the last few characters are the Base58Check checksum.

Usage:
    python3 contrib/generate_burn_addresses.py [--version VERSION_BYTE]
    python3 contrib/generate_burn_addresses.py --validate

Default version byte is 78 (mainnet Y-prefix addresses).
Use --version 111 for testnet (n-prefix addresses).
"""

import hashlib
import sys

B58_ALPHABET = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz'


def b58decode(s):
    """Decode a Base58 string to bytes (no checksum handling)."""
    value = 0
    for c in s:
        value = value * 58 + B58_ALPHABET.index(c)
    result = value.to_bytes((value.bit_length() + 7) // 8, 'big') if value else b''
    # Preserve leading '1' characters as 0x00 bytes
    num_leading = 0
    for c in s:
        if c == '1':
            num_leading += 1
        else:
            break
    return b'\x00' * num_leading + result


def b58encode(data):
    """Encode bytes to a Base58 string (no checksum handling)."""
    value = int.from_bytes(data, 'big')
    result = ''
    while value > 0:
        value, mod = divmod(value, 58)
        result = B58_ALPHABET[mod] + result
    # Preserve leading 0x00 bytes as '1' characters
    for byte in data:
        if byte == 0:
            result = '1' + result
        else:
            break
    return result


def sha256d(data):
    """Double SHA-256 hash."""
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()


def checksum(payload):
    """Compute 4-byte Base58Check checksum."""
    return sha256d(payload)[:4]


def b58check_encode(raw_21):
    """Encode a 21-byte payload (version + 20 bytes) as Base58Check."""
    assert len(raw_21) == 21
    return b58encode(raw_21 + checksum(raw_21))


def b58check_decode(address):
    """Decode a Base58Check address. Returns (version, payload_20) or raises."""
    raw = b58decode(address)
    if len(raw) != 25:
        raise ValueError(f"Decoded to {len(raw)} bytes, expected 25")
    data, cs = raw[:-4], raw[-4:]
    expected_cs = checksum(data)
    if cs != expected_cs:
        raise ValueError(
            f"Checksum mismatch: got {cs.hex()}, expected {expected_cs.hex()}")
    return data[0], data[1:]


def generate_burn_address(desired_prefix, version_byte):
    """
    Generate a valid burn address whose Base58 encoding starts with
    the desired_prefix, padded with 'X' characters in the middle.

    Algorithm:
      1. Build a full-length address string: desired_prefix + 'X' * padding
      2. Base58-decode to raw bytes
      3. Set byte[0] to the correct version byte
      4. Pad/truncate to exactly 21 bytes
      5. Compute SHA256d checksum (4 bytes)
      6. Base58-encode the 25-byte result

    The last ~5-6 characters of the result are determined by the checksum,
    so they will differ from 'X'. The readable prefix is preserved.
    """
    # A P2PKH address is 25 raw bytes → typically 33-34 Base58 chars.
    # We target 34 characters for padding (standard length).
    target_len = 34
    if len(desired_prefix) > target_len - 4:
        raise ValueError(f"Prefix '{desired_prefix}' too long (max {target_len - 4} chars)")

    padded = desired_prefix + 'X' * (target_len - len(desired_prefix))

    # Decode the full padded string to raw bytes
    raw = b58decode(padded)

    # Force version byte and ensure exactly 21 bytes
    if len(raw) >= 21:
        raw = bytes([version_byte]) + raw[1:21]
    else:
        raw = bytes([version_byte]) + raw[1:] + b'\x00' * (21 - len(raw))

    # Compute valid checksum and encode
    address = b58check_encode(raw)
    return address


def validate_address(address, expected_version):
    """Validate a Base58Check address. Returns (is_valid, version, detail)."""
    try:
        version, payload = b58check_decode(address)
        if version != expected_version:
            return False, version, f"Wrong version byte: {version} (expected {expected_version})"
        return True, version, "Valid"
    except ValueError as e:
        return False, None, str(e)


# Desired address prefixes for Yottaflux burn addresses.
# These are the human-readable prefixes that will appear in the address.
# The rest is padded with X's, and the last ~5 chars become the checksum.
#
# Note: With version byte 78, the valid second character range is 'P'-'n'.
# Characters outside this range (like uppercase 'A'-'N') cannot appear as
# the second character of a valid address, so prefixes use lowercase where
# needed (e.g. "Yburn" instead of "YBurn").
BURN_ADDRESSES = {
    'strIssueAssetBurnAddress':             'YissueAssetXXXXXXXXXXXXXXXXXXX',
    'strReissueAssetBurnAddress':           'YReissueAssetXXXXXXXXXXXXXXXXX',
    'strIssueSubAssetBurnAddress':          'YissueSubAssetXXXXXXXXXXXXXXXX',
    'strIssueUniqueAssetBurnAddress':       'YissueUniqueAssetXXXXXXXXXXXXX',
    'strIssueMsgChannelAssetBurnAddress':   'YissueMsgChanneLAssetXXXXXXXXX',
    'strIssueQualifierAssetBurnAddress':    'YissueQuaLifierXXXXXXXXXXXXXXX',
    'strIssueSubQualifierAssetBurnAddress': 'YissueSubQuaLifierXXXXXXXXXXXX',
    'strIssueRestrictedAssetBurnAddress':   'YissueRestrictedXXXXXXXXXXXXXX',
    'strAddNullQualifierTagBurnAddress':    'YaddTagBurnXXXXXXXXXXXXXXXXXXX',
    'strGlobalBurnAddress':                 'YburnXXXXXXXXXXXXXXXXXXXXXXXXX',
}


def main():
    version_byte = 78  # Mainnet Y-prefix
    mode = "generate"

    # Simple arg parsing
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == '--version':
            version_byte = int(args[i + 1])
            i += 2
        elif args[i] == '--validate':
            mode = "validate"
            i += 1
        elif args[i] in ('--help', '-h'):
            print(__doc__)
            sys.exit(0)
        else:
            print(f"Unknown argument: {args[i]}", file=sys.stderr)
            sys.exit(1)

    if mode == "generate":
        print(f"Generating burn addresses with version byte {version_byte}:")
        print()

        max_name_len = max(len(name) for name in BURN_ADDRESSES)
        for param_name, prefix in BURN_ADDRESSES.items():
            address = generate_burn_address(prefix, version_byte)
            is_valid, ver, detail = validate_address(address, version_byte)
            assert is_valid, f"Generated address {address} failed validation: {detail}"
            print(f'        {param_name:{max_name_len}s} = "{address}";')

        print()
        print("All addresses validated successfully.")
        print()

        # Also print a validation pass
        print("Verification:")
        for param_name, prefix in BURN_ADDRESSES.items():
            address = generate_burn_address(prefix, version_byte)
            is_valid, ver, detail = validate_address(address, version_byte)
            print(f"  [{('VALID' if is_valid else 'INVALID'):7s}] {address}  ver={ver}")

    elif mode == "validate":
        print(f"Validating current burn addresses against version byte {version_byte}:")
        print()

        # Current mainnet addresses from chainparams.cpp
        current_addresses = {
            'strIssueAssetBurnAddress':             'YissueAssetXXXXXXXXXXXXXXXXXW8oK1h',
            'strReissueAssetBurnAddress':           'YReissueAssetXXXXXXXXXXXXXXXYcNAB6',
            'strIssueSubAssetBurnAddress':          'YissueSubAssetXXXXXXXXXXXXXXcAjBNU',
            'strIssueUniqueAssetBurnAddress':       'YissueUniqueAssetXXXXXXXXXXXZAr1F6',
            'strIssueMsgChannelAssetBurnAddress':   'YissueMsgChanneLAssetXXXXXXXdbjHqe',
            'strIssueQualifierAssetBurnAddress':    'YissueQuaLifierXXXXXXXXXXXXXTQvwL8',
            'strIssueSubQualifierAssetBurnAddress': 'YissueSubQuaLifierXXXXXXXXXXYJchwm',
            'strIssueRestrictedAssetBurnAddress':   'YissueRestrictedXXXXXXXXXXXXUkSk3r',
            'strAddNullQualifierTagBurnAddress':    'YaddTagBurnXXXXXXXXXXXXXXXXXZJAYt2',
            'strGlobalBurnAddress':                 'YburnXXXXXXXXXXXXXXXXXXXXXXXYqtbxJ',
        }

        for param_name, address in current_addresses.items():
            is_valid, ver, detail = validate_address(address, version_byte)
            status = "VALID" if is_valid else "INVALID"
            ver_str = f"(ver={ver})" if ver is not None else ""
            print(f"  [{status:7s}] {param_name}: {address}  {ver_str} {detail}")


if __name__ == '__main__':
    main()
