#!/usr/bin/env python3
"""
Génère une vingtaine de fichiers .pcapng, chacun avec une seule frame
RadioTap (DLT 127 = LINKTYPE_IEEE802_11_RADIOTAP), pour tester un parser.

Certains fichiers sont valides, la majorité contient des corruptions
volontaires (header RadioTap et/ou payload 802.11).
"""
import struct
import os

OUT_DIR = "/mnt/user-data/outputs"
os.makedirs(OUT_DIR, exist_ok=True)

DLT_IEEE802_11_RADIOTAP = 127

# ---------------------------------------------------------------------------
# pcapng low-level block builders
# ---------------------------------------------------------------------------

def pad4(b: bytes) -> bytes:
    n = (-len(b)) % 4
    return b + b"\x00" * n


def block(block_type: int, body: bytes) -> bytes:
    """Wrap a block body with type + total_length ... total_length trailer."""
    body = pad4(body)
    total_len = 12 + len(body)  # type(4) + totlen(4) + body + totlen(4)
    return (
        struct.pack("<I", block_type)
        + struct.pack("<I", total_len)
        + body
        + struct.pack("<I", total_len)
    )


def section_header_block() -> bytes:
    body = struct.pack("<IHHq", 0x1A2B3C4D, 1, 0, -1)  # magic, major, minor, section_len=-1
    return block(0x0A0D0D0A, body)


def interface_description_block(linktype=DLT_IEEE802_11_RADIOTAP, snaplen=0xFFFF) -> bytes:
    body = struct.pack("<HHI", linktype, 0, snaplen)
    return block(0x00000001, body)


def enhanced_packet_block(data: bytes, iface_id=0, ts=0x0000000000000001) -> bytes:
    ts_high = (ts >> 32) & 0xFFFFFFFF
    ts_low = ts & 0xFFFFFFFF
    caplen = len(data)
    origlen = len(data)
    body = struct.pack("<IIIII", iface_id, ts_high, ts_low, caplen, origlen) + data
    return block(0x00000006, body)


def write_pcapng(path: str, packet_data: bytes):
    out = section_header_block() + interface_description_block() + enhanced_packet_block(packet_data)
    with open(path, "wb") as f:
        f.write(out)


# ---------------------------------------------------------------------------
# RadioTap helpers
# ---------------------------------------------------------------------------
# Present bit numbers we use
RT_TSFT = 0
RT_FLAGS = 1
RT_RATE = 2
RT_CHANNEL = 3
RT_DBM_ANTSIGNAL = 5
RT_ANTENNA = 11
RT_EXT = 31  # extended presence word follows


def present_mask(bits):
    m = 0
    for b in bits:
        m |= (1 << b)
    return m


def radiotap_valid(channel_freq=2437, channel_flags=0x00A0, dbm=-42, rate=0x02,
                    antenna=0, flags=0x00):
    """A well-formed RadioTap v0 header: Flags, Rate, Channel, dBm AntSignal, Antenna."""
    present = present_mask([RT_FLAGS, RT_RATE, RT_CHANNEL, RT_DBM_ANTSIGNAL, RT_ANTENNA])
    fields = struct.pack("<BB", flags, rate)                     # Flags(u8) Rate(u8) -> offsets 8,9
    fields += struct.pack("<HH", channel_freq, channel_flags)    # Channel u16+u16   -> offsets 10-13
    fields += struct.pack("<b", dbm)                             # dBm AntSignal s8  -> offset 14
    fields += struct.pack("<B", antenna)                         # Antenna u8        -> offset 15
    it_len = 8 + len(fields)
    header = struct.pack("<BBHI", 0, 0, it_len, present) + fields
    assert len(header) == it_len
    return header


# ---------------------------------------------------------------------------
# 802.11 helpers
# ---------------------------------------------------------------------------

def mac(s):
    return bytes.fromhex(s.replace(":", ""))


def beacon_frame(ssid=b"test", full_ies=True, bad_ssid_len=False,
                  truncate_after=None, bad_fc_type=False, bad_subtype=False,
                  missing_fixed_fields=False):
    if bad_fc_type:
        fc = struct.pack("<BB", 0x8C, 0x00)   # type=3 (reserved), subtype=8
    elif bad_subtype:
        fc = struct.pack("<BB", 0xF0, 0x00)   # type=0 (mgmt), subtype=0xF (reserved)
    else:
        fc = struct.pack("<BB", 0x80, 0x00)   # type=0 mgmt, subtype=8 beacon

    duration = struct.pack("<H", 0)
    da = mac("ff:ff:ff:ff:ff:ff")
    sa = mac("02:00:00:00:00:01")
    bssid = sa
    seq = struct.pack("<H", 0)

    mac_header = fc + duration + da + sa + bssid + seq

    if missing_fixed_fields:
        # timestamp/beacon-interval/capability fixed params entirely absent
        return mac_header

    fixed = struct.pack("<Q", 0) + struct.pack("<H", 100) + struct.pack("<H", 0x0001)

    if bad_ssid_len:
        # IE claims length 32 but only 4 bytes of actual SSID data follow
        ssid_ie = struct.pack("<BB", 0, 32) + ssid
    else:
        ssid_ie = struct.pack("<BB", 0, len(ssid)) + ssid

    rates_ie = struct.pack("<BB", 1, 4) + bytes([0x82, 0x84, 0x8B, 0x96])
    ds_ie = struct.pack("<BB", 3, 1) + bytes([6])

    frame = mac_header + fixed + ssid_ie
    if full_ies:
        frame += rates_ie + ds_ie

    if truncate_after is not None:
        frame = frame[:truncate_after]

    return frame


# ---------------------------------------------------------------------------
# Case generation
# ---------------------------------------------------------------------------

cases = {}

def add(name, packet):
    cases[name] = packet

# 1-2: fully valid ---------------------------------------------------------
add("01_valid_v0_minimal", radiotap_valid() + beacon_frame(full_ies=False))
add("02_valid_v0_full_beacon", radiotap_valid() + beacon_frame(full_ies=True))

# 3: present bitmask incohérent (annonce Channel+dBm mais octets absents) --
present = present_mask([RT_FLAGS, RT_RATE, RT_CHANNEL, RT_DBM_ANTSIGNAL])
fields = struct.pack("<BB", 0x00, 0x02)  # seulement Flags+Rate fournis
it_len = 8 + len(fields)  # it_len cohérent avec ce qui est VRAIMENT écrit, pas avec le bitmask
rt = struct.pack("<BBHI", 0, 0, it_len, present) + fields
add("03_corrupt_present_bitmask_mismatch", rt + beacon_frame())

# 4: it_len trop court (tronque le header au milieu du champ Channel) -----
good = radiotap_valid()
bad_it_len = 11  # coupe en plein milieu du champ Channel (qui commence à l'offset 10)
rt = good[:2] + struct.pack("<H", bad_it_len) + good[4:]
add("04_corrupt_itlen_too_short", rt + beacon_frame())

# 5: it_len trop long (dépasse la taille réelle des données capturées) ----
good = radiotap_valid()
bad_it_len = len(good) + 40  # prétend couvrir 40 octets de plus qu'il n'y en a
rt = good[:2] + struct.pack("<H", bad_it_len) + good[4:]
add("05_corrupt_itlen_too_long", rt + beacon_frame())

# 6: dBm Antenna Signal aberrant (hors plage réaliste, ex +80 dBm) --------
add("06_corrupt_dbm_out_of_range", radiotap_valid(dbm=80) + beacon_frame())

# 7: fréquence de canal invalide (0xFFFF) ----------------------------------
add("07_corrupt_channel_freq_invalid", radiotap_valid(channel_freq=0xFFFF, channel_flags=0xFFFF) + beacon_frame())

# 8: alignement cassé (Channel censé être aligné sur 2 mais décalé) -------
present = present_mask([RT_FLAGS, RT_CHANNEL])
fields = struct.pack("<B", 0x00)                 # Flags (1 octet, offset 8)
fields += struct.pack("<HH", 2437, 0x00A0)       # Channel démarre à l'offset 9 (non aligné, pas de padding)
it_len = 8 + len(fields)
rt = struct.pack("<BBHI", 0, 0, it_len, present) + fields
add("08_corrupt_alignment_broken", rt + beacon_frame())

# 9: TSFT (u64, doit faire 8 octets) tronqué à 4 octets --------------------
present = present_mask([RT_TSFT])
fields = struct.pack("<I", 0x12345678)  # seulement 4 des 8 octets attendus
it_len = 8 + len(fields)
rt = struct.pack("<BBHI", 0, 0, it_len, present) + fields
add("09_corrupt_truncated_mid_tsft", rt + beacon_frame())

# 10: capture coupée en plein milieu du header fixe (8 octets) ------------
add("10_corrupt_truncated_header_only", radiotap_valid()[:3])  # ni it_len ni present complets

# 11: it_version invalide (seule la version 0 est définie) ----------------
good = radiotap_valid()
rt = struct.pack("<B", 5) + good[1:]
add("11_invalid_version", rt + beacon_frame())

# 12: SSID IE annonce une longueur supérieure aux données réelles ---------
add("12_valid_beacon_bad_ssid_len", radiotap_valid() + beacon_frame(bad_ssid_len=True))

# 13: sous-type de frame management réservé/invalide -----------------------
add("13_valid_management_bad_subtype", radiotap_valid() + beacon_frame(bad_subtype=True))

# 14: type de frame réservé (FC.type = 3) -----------------------------------
add("14_valid_beacon_wrong_fc_type", radiotap_valid() + beacon_frame(bad_fc_type=True))

# 15: frame tronquée en plein milieu d'un IE (Supported Rates coupé) -------
full = beacon_frame(full_ies=True)
# coupe 3 octets avant la fin (en plein milieu de l'IE Supported Rates)
add("15_valid_beacon_truncated_mid_ie", radiotap_valid() + full[:-3])

# 16: champs fixes obligatoires du beacon totalement absents ---------------
add("16_valid_beacon_missing_essential_fields",
    radiotap_valid() + beacon_frame(missing_fixed_fields=True))

# 17: combo -- it_len RadioTap erroné + SSID IE corrompu --------------------
good = radiotap_valid()
bad_it_len = 12
rt = good[:2] + struct.pack("<H", bad_it_len) + good[4:]
add("17_corrupt_radiotap_and_payload_combo1", rt + beacon_frame(bad_ssid_len=True))

# 18: combo -- version RadioTap invalide + FC.type réservé ------------------
good = radiotap_valid()
rt = struct.pack("<B", 7) + good[1:]
add("18_corrupt_radiotap_and_payload_combo2", rt + beacon_frame(bad_fc_type=True))

# 19: edge case -- header RadioTap valide mais payload 802.11 vide ---------
add("19_edge_empty_payload_after_radiotap", radiotap_valid())

# 20: edge case -- bit d'extension du present (bit31) posé mais le 2e mot
#     de présence est absent/tronqué ----------------------------------------
present = present_mask([RT_FLAGS]) | (1 << RT_EXT)
fields = struct.pack("<B", 0x00)  # Flags fourni mais PAS de 2e mot de bitmask "extended"
it_len = 8 + len(fields)
rt = struct.pack("<BBHI", 0, 0, it_len, present) + fields
add("20_edge_extended_present_bit_no_second_word", rt + beacon_frame())

# ---------------------------------------------------------------------------
# Write everything out
# ---------------------------------------------------------------------------

for name, pkt in sorted(cases.items()):
    path = os.path.join(OUT_DIR, f"{name}.pcapng")
    write_pcapng(path, pkt)
    print(f"{name:50s} {len(pkt):4d} bytes")

print(f"\n{len(cases)} fichiers générés dans {OUT_DIR}")
