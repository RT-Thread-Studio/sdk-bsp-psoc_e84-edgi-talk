/* SPDX-License-Identifier: Apache-2.0 */
#include <rtthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cy_rram.h"
#include "cymem_CM55_0.h"
#include "btstack_util.h"
#include "edgi_hid_store.h"
#if (CY_CPU_CORTEX_M55)
#include <rthw.h>
#endif

/* Reserve the last 8 KiB of the BSP's 32 KiB nonsecure, M33/M55-shared
 * user NVM. M55 uses the C-bus alias; the official M33 KV example uses
 * the first 8 KiB of that same region. RRAM writes do not require erase. */
#define SLOT_SIZE   4096u
#define SLOT_BASE   (CYMEM_CM55_0_user_nvm_C_START + CYMEM_CM55_0_user_nvm_SIZE - 2u * SLOT_SIZE)
#define COMMIT_TAG  0x31444948u  /* HID1, little endian */
#define DATA_OFFSET 32u
#define PREFIX_SIZE 32u          /* addresses, key, type, reserved byte, map length */

static const uint8_t owner[16] = "Edgi-HID-Bond-v1";
/* v1 Classic records end before this offset even with a 2048-byte map.
 * Both LE copies fit in one physical slot when the other belongs to another user. */
#define LE_AREA_OFFSET  0x840u
#define LE_COPY_STRIDE  128u
#define LE_DATA_SIZE    64u
#define LE_COMMIT_TAG   0x31454c48u /* HLE1 */
static const uint8_t le_owner[16] = "Edgi-HID-LE-v1";
#define BONDS_AREA_OFFSET (LE_AREA_OFFSET + sizeof(le_owner) + 2 * LE_COPY_STRIDE)
#define BONDS_HEADER_SIZE 16u
#define BOND_DATA_SIZE    96u
#define BONDS_DATA_SIZE   (BONDS_HEADER_SIZE + CLASSIC_HID_MAX_BONDS * BOND_DATA_SIZE)
#define BONDS_COPY_STRIDE (sizeof(struct commit) + BONDS_DATA_SIZE)
#define BONDS_COMMIT_TAG  0x32444948u /* HID2 */
#define BOND_CLASSIC_TYPE 0xffu
static const uint8_t bonds_owner[16] = "Edgi-HID-List-v2";
static const uint8_t zeros[64];
/* The Classic host thread serializes calls to this provider. */
static uint8_t payload[PREFIX_SIZE + CLASSIC_HID_MAX_DESCRIPTOR];
static uint8_t bonds_payload[BONDS_DATA_SIZE];

/* Previous firmware kept one record per transport. Read its wire format for
 * migration. Classic report maps are rediscovered when connecting. */
struct classic_hid_bond
{
    uint8_t local[6], peer[6], link_key[16], key_type;
};

struct hid_le_preference
{
    uint8_t local[6], peer[6], address_type;
    struct hid_le_bond bond;
};

struct commit
{
    uint32_t tag, sequence, size, crc;
};

enum ownership { AVAILABLE, OWNED, FOREIGN, UNKNOWN_OWNER };

struct slot
{
    uint32_t base;
    enum ownership ownership;
    bool valid;
    struct commit record;
};

RT_STATIC_ASSERT(hid_commit_block, sizeof(struct commit) == CY_RRAM_BLOCK_SIZE_BYTES);
RT_STATIC_ASSERT(hid_bond_fits_classic_area, DATA_OFFSET + sizeof(payload) <= LE_AREA_OFFSET);
RT_STATIC_ASSERT(hid_le_copy_fits, sizeof(struct commit) + LE_DATA_SIZE <= LE_COPY_STRIDE);
RT_STATIC_ASSERT(hid_scrub_buffer_fits, LE_DATA_SIZE <= sizeof(zeros) && PREFIX_SIZE <= sizeof(zeros));
RT_STATIC_ASSERT(hid_le_area_fits, LE_AREA_OFFSET + sizeof(le_owner) + 2 * LE_COPY_STRIDE <= SLOT_SIZE);
RT_STATIC_ASSERT(hid_bonds_area_fits, BONDS_AREA_OFFSET + sizeof(bonds_owner) + 2 * BONDS_COPY_STRIDE <= SLOT_SIZE);
RT_STATIC_ASSERT(hid_region_fits, CYMEM_CM55_0_user_nvm_SIZE >= 2u * SLOT_SIZE);
RT_STATIC_ASSERT(hid_region_mpu_aligned, ((SLOT_BASE | SLOT_SIZE) & 31u) == 0);

#if (CY_CPU_CORTEX_M55)
static bool rram_mapped;

static int prepare_rram_region(void)
{
    if (rram_mapped) return 0;

    /* The RRAM host window fills the controller's write buffer during an
     * indirect write. Device-nGnRnE keeps the CM55 from caching, combining
     * or posting those writes. Map only the HID journal's reserved area. */
    rt_base_t level = rt_hw_interrupt_disable();
    uint32_t previous = MPU->RNR;
    unsigned count = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    unsigned slot = count;
    unsigned used_attrs = 0;
    bool overlaps = false;
    for (unsigned i = 0; i < count; ++i)
    {
        MPU->RNR = i;
        uint32_t limit = MPU->RLAR;
        if (limit & MPU_RLAR_EN_Msk)
        {
            uint32_t first = MPU->RBAR & MPU_RBAR_BASE_Msk;
            uint32_t last = (limit & MPU_RLAR_LIMIT_Msk) | 31u;
            overlaps |= first < SLOT_BASE + 2u * SLOT_SIZE && last >= SLOT_BASE;
            used_attrs |= 1u << ((limit & MPU_RLAR_AttrIndx_Msk) >> MPU_RLAR_AttrIndx_Pos);
        }
        else if (slot == count)
            slot = i;
    }
    unsigned attr = 0;
    while (attr < 8 && (used_attrs & (1u << attr))) ++attr;
    if ((MPU->CTRL & MPU_CTRL_ENABLE_Msk) && !overlaps && slot < count && attr < 8)
    {
        SCB_InvalidateDCache_by_Addr((void *)SLOT_BASE, 2u * SLOT_SIZE);
        ARM_MPU_SetMemAttr(attr, ARM_MPU_ATTR(0u, ARM_MPU_ATTR_DEVICE_nGnRnE));
        ARM_MPU_SetRegion(slot, ARM_MPU_RBAR(SLOT_BASE, ARM_MPU_SH_NON, 0u, 0u, 1u),
                         ARM_MPU_RLAR(SLOT_BASE + 2u * SLOT_SIZE - 1u, attr));
        rram_mapped = true;
    }
    MPU->RNR = previous;
    __DSB();
    __ISB();
    rt_hw_interrupt_enable(level);
    if (rram_mapped) return 0;
    rt_kprintf("Bluetooth HID could not configure the RRAM MPU region\n");
    return -RT_EBUSY;
}
#endif

static int read_bytes(uint32_t addr, void *buf, size_t n)
{
    /* The tearing-safe PDL checks ECC before dereferencing a block. An
     * interrupted write can leave a block which must not be read directly. */
    cy_en_rram_status_t status = Cy_RRAM_TSReadByteArray(RRAMC0, addr, buf, n);
    if (status == CY_RRAM_ECC_FAIL) return -RT_ERROR;
    return status == CY_RRAM_SUCCESS ? 0 : -RT_EIO;
}

static int write_bytes(uint32_t addr, const void *buf, size_t n)
{
    cy_en_rram_status_t status = Cy_RRAM_TSWriteByteArray(RRAMC0, addr, buf, n);
    if (status == CY_RRAM_SUCCESS) return 0;
    rt_kprintf("Bluetooth HID RRAM write failed at 0x%08lx (0x%08lx)\n",
               (unsigned long)addr, (unsigned long)status);
    return -RT_EIO;
}

static uint32_t checksum_data(const struct commit *record, const uint8_t *data)
{
    uint32_t crc = btstack_crc32_update(btstack_crc32_init(), (const uint8_t *)record,
                                        offsetof(struct commit, crc));
    return btstack_crc32_finalize(btstack_crc32_update(crc, data, record->size));
}

static uint32_t checksum(const struct commit *record)
{
    return checksum_data(record, payload);
}

static int inspect_owner(uint32_t base, unsigned size, const uint8_t expected[16], enum ownership *result)
{
    uint8_t data[sizeof(zeros)];
    *result = FOREIGN;
    int rc = read_bytes(base, data, sizeof(owner));
    if (rc == -RT_ERROR)
    {
        *result = UNKNOWN_OWNER;
        return 0;
    }
    if (rc) return rc;
    if (memcmp(data, expected, sizeof(owner)))
    {
        /* A reset during the initial 16-byte ownership write can leave its
         * prefix followed by pristine zeros. No payload was started yet.
         * Anything else, anywhere in the slot, belongs to another user. */
        unsigned i = 0;
        while (i < sizeof(owner) && data[i] == expected[i]) i++;
        if (memcmp(data + i, zeros, sizeof(owner) - i)) return 0;
        for (unsigned offset = sizeof(owner); offset < size; offset += sizeof(data))
        {
            unsigned n = size - offset;
            if (n > sizeof(data)) n = sizeof(data);
            rc = read_bytes(base + offset, data, n);
            if (rc == -RT_ERROR) return 0;
            if (rc) return rc;
            if (memcmp(data, zeros, n)) return 0;
        }
        *result = AVAILABLE;
        return 0;
    }
    *result = OWNED;
    return 0;
}

static int inspect(struct slot *slot, unsigned index)
{
    slot->base = SLOT_BASE + index * SLOT_SIZE;
    slot->valid = false;
    int rc = inspect_owner(slot->base, SLOT_SIZE, owner, &slot->ownership);
    if (rc || slot->ownership != OWNED) return rc;
    struct commit *record = &slot->record;
    rc = read_bytes(slot->base + sizeof(owner), record, sizeof(*record));
    if (rc == -RT_ERROR) return 0; /* Owned but torn. The previous slot is authoritative. */
    if (rc) return rc;
    if (record->tag != COMMIT_TAG || record->size > sizeof(payload)) return 0;
    if (record->size)
    {
        if (record->size < PREFIX_SIZE) return 0;
        rc = read_bytes(slot->base + DATA_OFFSET, payload, record->size);
        if (rc == -RT_ERROR) return 0;
        if (rc) return rc;
        if (little_endian_read_16(payload, 30) != record->size - PREFIX_SIZE) return 0;
    }
    slot->valid = record->crc == checksum(record);
    return 0;
}

static int scan(struct slot slots[2], int *active)
{
    if (inspect(&slots[0], 0) || inspect(&slots[1], 1)) return -RT_EIO;
    if (!slots[0].valid) *active = slots[1].valid ? 1 : -1;
    else if (!slots[1].valid) *active = 0;
    else *active = (int32_t)(slots[1].record.sequence - slots[0].record.sequence) > 0 ? 1 : 0;
    return 0;
}

static int scrub_bytes(uint32_t addr, size_t size)
{
    uint8_t data[sizeof(zeros)];
    int rc = read_bytes(addr, data, size);
    if (rc && rc != -RT_ERROR) return rc;
    if (rc || memcmp(data, zeros, size))
    {
        if (write_bytes(addr, zeros, size) || read_bytes(addr, data, size) ||
            memcmp(data, zeros, size)) return -RT_EIO;
    }
    return 0;
}

static int scrub_keys(const struct slot slots[2])
{
    /* After committing a tombstone, wipe both former key locations. Retry
     * after a reset if power failed between committing and clearing the keys. */
    for (unsigned i = 0; i < 2; i++)
    {
        if (slots[i].ownership != OWNED) continue;
        int rc = scrub_bytes(slots[i].base + DATA_OFFSET, PREFIX_SIZE);
        if (rc) return rc;
    }
    return 0;
}

static int load_bond(struct classic_hid_bond *out)
{
    struct slot slots[2];
    int active;
    if (!out) return -RT_EINVAL;
    if (scan(slots, &active)) return -RT_EIO;
    if (active < 0)
    {
        if (slots[0].ownership == UNKNOWN_OWNER || slots[1].ownership == UNKNOWN_OWNER) return -RT_EIO;
        if (slots[0].ownership == FOREIGN && slots[1].ownership == FOREIGN) return -RT_EBUSY;
    }
    if (active < 0 || !slots[active].record.size)
    {
        /* A torn in-place tombstone can leave an owned key with no valid
         * commit. Finish the scrub before reporting an empty binding. */
        if (scrub_keys(slots)) return -RT_EIO;
        return -RT_EEMPTY;
    }
    struct slot *slot = &slots[active];
    if (read_bytes(slot->base + DATA_OFFSET, payload, slot->record.size) ||
        checksum(&slot->record) != slot->record.crc) return -RT_EIO;
    memcpy(out->local, payload, 6);
    memcpy(out->peer, payload + 6, 6);
    memcpy(out->link_key, payload + 12, 16);
    out->key_type = payload[28];
    return 0;
}

static int clear_bond(void)
{
    struct slot slots[2];
    int active;
    if (scan(slots, &active)) return -RT_EIO;
    /* An unreadable owner could conceal another HID generation. Its sequence
     * is unknown, so a completed forget cannot safely supersede it. */
    if (slots[0].ownership == UNKNOWN_OWNER || slots[1].ownership == UNKNOWN_OWNER) return -RT_EIO;
    if (active < 0 || !slots[active].record.size) return scrub_keys(slots);

    unsigned target = active < 0 ? 0u : 1u - (unsigned)active;
    if (slots[target].ownership == FOREIGN)
    {
        target = 1u - target;
        if (slots[target].ownership == FOREIGN) return -RT_EBUSY;
    }
    struct slot *slot = &slots[target];
    struct commit record = {COMMIT_TAG, active < 0 ? 1u : slots[active].record.sequence + 1u, 0, 0};
    if (slot->ownership == AVAILABLE && write_bytes(slot->base, owner, sizeof(owner))) return -RT_EIO;
    uint32_t commit_addr = slot->base + sizeof(owner);
    record.crc = checksum(&record);
    if (write_bytes(commit_addr, &record, sizeof(record)) || inspect(slot, target) ||
        !slot->valid || memcmp(&record, &slot->record, sizeof(record))) return -RT_EIO;
    return scrub_keys(slots);
}

struct le_copy
{
    struct commit record;
    uint8_t data[LE_DATA_SIZE];
};

struct le_journal
{
    uint32_t base; /* First commit, immediately after le_owner. */
    struct le_copy copies[2];
    int active;
};

/* Reopen the same journal even if the other physical slot becomes available
 * later. Before first use, require a blank area and claim the outer slot first. */
static int locate_journal(unsigned offset, unsigned stride, const uint8_t marker[16], bool create, uint32_t *base)
{
    uint32_t existing = 0, spare = 0;
    bool spare_owned = false;
    for (unsigned i = 0; i < 2; ++i)
    {
        uint32_t slot = SLOT_BASE + i * SLOT_SIZE;
        enum ownership outer, inner = AVAILABLE;
        if (inspect_owner(slot, SLOT_SIZE, owner, &outer) || outer == UNKNOWN_OWNER)
            return -RT_EIO;
        if (outer == FOREIGN) continue;
        uint32_t area = slot + offset;
        if (outer == OWNED &&
            (inspect_owner(area, sizeof(le_owner) + 2 * stride, marker, &inner) ||
             inner == UNKNOWN_OWNER))
            return -RT_EIO;
        if (inner == OWNED)
        {
            if (existing) return -RT_EIO;
            existing = area;
        }
        else if (inner == AVAILABLE && (!spare || (!spare_owned && outer == OWNED)))
        {
            spare = area;
            spare_owned = outer == OWNED;
        }
    }
    if (!existing)
    {
        if (!create) return -RT_EEMPTY;
        if (!spare) return -RT_EBUSY;
        if (!spare_owned && write_bytes(spare - offset, owner, sizeof(owner))) return -RT_EIO;
        if (write_bytes(spare, marker, sizeof(le_owner))) return -RT_EIO;
        existing = spare;
    }
    *base = existing + sizeof(le_owner);
    return 0;
}

static int scan_le(struct le_journal *j)
{
    int rc = locate_journal(LE_AREA_OFFSET, LE_COPY_STRIDE, le_owner, false, &j->base);
    if (rc) return rc;
    j->active = -1;
    for (int i = 0; i < 2; ++i)
    {
        uint32_t addr = j->base + i * LE_COPY_STRIDE;
        struct le_copy *c = &j->copies[i];
        rc = read_bytes(addr, &c->record, sizeof(c->record));
        if (rc == -RT_ERROR) continue; /* A torn copy leaves its predecessor intact. */
        if (rc) return rc;
        if (c->record.tag != LE_COMMIT_TAG || (c->record.size && c->record.size != LE_DATA_SIZE))
            continue;
        if (c->record.size)
        {
            rc = read_bytes(addr + sizeof(c->record), c->data, LE_DATA_SIZE);
            if (rc == -RT_ERROR) continue;
            if (rc) return rc;
        }
        if (c->record.crc != checksum_data(&c->record, c->data)) continue;
        if (j->active < 0 || (int32_t)(c->record.sequence - j->copies[j->active].record.sequence) > 0)
            j->active = i;
    }
    return 0;
}

/* The wire format is independent of C structure alignment and host endianness. */
static void encode_le(uint8_t data[LE_DATA_SIZE], const struct hid_le_preference *p)
{
    memset(data, 0, LE_DATA_SIZE);
    memcpy(data, p->local, 6);
    memcpy(data + 6, p->peer, 6);
    data[12] = p->address_type;
    if (!p->bond.key_size) return;
    data[13] = p->bond.identity_type;
    memcpy(data + 14, p->bond.identity, 6);
    data[20] = p->bond.key_size;
    data[21] = p->bond.flags;
    little_endian_store_16(data, 22, p->bond.ediv);
    memcpy(data + 24, p->bond.irk, 16);
    memcpy(data + 40, p->bond.ltk, 16);
    memcpy(data + 56, p->bond.rand, 8);
}

static void decode_le(struct hid_le_preference *p, const uint8_t data[LE_DATA_SIZE])
{
    memset(p, 0, sizeof(*p));
    memcpy(p->local, data, 6);
    memcpy(p->peer, data + 6, 6);
    p->address_type = data[12];
    p->bond.identity_type = data[13];
    memcpy(p->bond.identity, data + 14, 6);
    p->bond.key_size = data[20];
    p->bond.flags = data[21];
    p->bond.ediv = little_endian_read_16(data, 22);
    memcpy(p->bond.irk, data + 24, 16);
    memcpy(p->bond.ltk, data + 40, 16);
    memcpy(p->bond.rand, data + 56, 8);
}

static int scrub_le(const struct le_journal *j)
{
    for (unsigned i = 0; i < 2; ++i)
    {
        uint32_t addr = j->base + i * LE_COPY_STRIDE + sizeof(struct commit);
        int rc = scrub_bytes(addr, LE_DATA_SIZE);
        if (rc) return rc;
    }
    return 0;
}

static int load_le(struct hid_le_preference *out)
{
    struct le_journal j;
    if (!out) return -RT_EINVAL;
    int rc = scan_le(&j);
    if (rc) return rc;
    if (j.active < 0 || !j.copies[j.active].record.size)
        return scrub_le(&j) ? -RT_EIO : -RT_EEMPTY;
    decode_le(out, j.copies[j.active].data);
    return 0;
}

static int clear_le(void)
{
    struct le_journal j;
    int rc = scan_le(&j);
    if (rc) return rc == -RT_EEMPTY ? 0 : rc;
    const struct le_copy *last = j.active < 0 ? NULL : &j.copies[j.active];
    if (!last || !last->record.size) return scrub_le(&j);
    struct commit next = {LE_COMMIT_TAG, last->record.sequence + 1u, 0, 0};
    next.crc = checksum_data(&next, zeros);
    uint32_t addr = j.base + (j.active == 0 ? LE_COPY_STRIDE : 0u);
    struct commit check;
    if (write_bytes(addr, &next, sizeof(next)) || read_bytes(addr, &check, sizeof(check)) ||
        memcmp(&next, &check, sizeof(next))) return -RT_EIO;
    return scrub_le(&j);
}

static int scan_bonds(uint32_t base, int *active, struct commit *record)
{
    *active = -1;
    for (int i = 0; i < 2; ++i)
    {
        uint32_t addr = base + i * BONDS_COPY_STRIDE;
        struct commit copy;
        int rc = read_bytes(addr, &copy, sizeof(copy));
        if (rc == -RT_ERROR) continue;
        if (rc) return rc;
        if (copy.tag != BONDS_COMMIT_TAG || copy.size != BONDS_DATA_SIZE) continue;
        rc = read_bytes(addr + sizeof(copy), bonds_payload, copy.size);
        if (rc == -RT_ERROR) continue;
        if (rc) return rc;
        if (bonds_payload[6] > CLASSIC_HID_MAX_BONDS || checksum_data(&copy, bonds_payload) != copy.crc)
            continue;
        if (*active < 0 || (int32_t)(copy.sequence - record->sequence) > 0)
        {
            *active = i;
            *record = copy;
        }
    }
    return 0;
}

static void encode_bonds(const struct classic_hid_bonds *records)
{
    memset(bonds_payload, 0, sizeof(bonds_payload));
    memcpy(bonds_payload, records->local, sizeof(records->local));
    bonds_payload[6] = (uint8_t)records->count;
    for (unsigned i = 0; i < records->count; ++i)
    {
        const struct classic_hid_bond_entry *entry = &records->entries[i];
        uint8_t *data = bonds_payload + BONDS_HEADER_SIZE + i * BOND_DATA_SIZE;
        if (entry->ble)
        {
            struct hid_le_preference le = { .address_type = entry->address_type, .bond = entry->keys.le };
            memcpy(le.peer, entry->address, sizeof(le.peer));
            encode_le(data, &le);
        }
        else
        {
            memcpy(data + 6, entry->address, sizeof(entry->address));
            data[12] = BOND_CLASSIC_TYPE;
            data[13] = entry->keys.classic.type;
            memcpy(data + 14, entry->keys.classic.key, sizeof(entry->keys.classic.key));
        }
        memcpy(data + LE_DATA_SIZE, entry->name, sizeof(entry->name));
    }
}

static void decode_bonds(struct classic_hid_bonds *records)
{
    memset(records, 0, sizeof(*records));
    memcpy(records->local, bonds_payload, sizeof(records->local));
    records->count = bonds_payload[6];
    for (unsigned i = 0; i < records->count; ++i)
    {
        struct classic_hid_bond_entry *entry = &records->entries[i];
        const uint8_t *data = bonds_payload + BONDS_HEADER_SIZE + i * BOND_DATA_SIZE;
        memcpy(entry->address, data + 6, sizeof(entry->address));
        memcpy(entry->name, data + LE_DATA_SIZE, sizeof(entry->name));
        entry->ble = data[12] != BOND_CLASSIC_TYPE;
        if (entry->ble)
        {
            struct hid_le_preference le;
            decode_le(&le, data);
            entry->address_type = le.address_type;
            entry->keys.le = le.bond;
        }
        else
        {
            entry->keys.classic.type = data[13];
            memcpy(entry->keys.classic.key, data + 14, sizeof(entry->keys.classic.key));
        }
    }
}

static int load_legacy(struct classic_hid_bonds *out)
{
    struct hid_le_preference le;
    struct classic_hid_bond classic;
    int le_rc = load_le(&le), classic_rc = load_bond(&classic);
    memset(out, 0, sizeof(*out));
    if (!le_rc)
    {
        struct classic_hid_bond_entry *entry = &out->entries[out->count++];
        entry->ble = true;
        entry->address_type = le.address_type;
        memcpy(entry->address, le.peer, sizeof(entry->address));
        entry->keys.le = le.bond;
        memcpy(out->local, le.local, sizeof(out->local));
    }
    if (!classic_rc && (le_rc || !memcmp(le.local, classic.local, sizeof(out->local))))
    {
        struct classic_hid_bond_entry *entry = &out->entries[out->count++];
        memcpy(entry->address, classic.peer, sizeof(entry->address));
        memcpy(entry->keys.classic.key, classic.link_key, sizeof(entry->keys.classic.key));
        entry->keys.classic.type = classic.key_type;
        memcpy(out->local, classic.local, sizeof(out->local));
    }
    if (out->count) return 0;
    return le_rc == -RT_EEMPTY ? classic_rc : le_rc;
}

static void scrub_obsolete_bonds(uint32_t addr)
{
    /* A verified commit supersedes this copy and both legacy journals.
     * Retry the cleanup when reopening after an interrupted scrub. */
    bool failed = false;
    for (unsigned offset = 0; offset < BONDS_DATA_SIZE; offset += sizeof(zeros))
    {
        unsigned n = BONDS_DATA_SIZE - offset;
        if (n > sizeof(zeros)) n = sizeof(zeros);
        if (scrub_bytes(addr + sizeof(struct commit) + offset, n)) failed = true;
    }
    int classic_rc = clear_bond(), le_rc = clear_le();
    if (failed || classic_rc || le_rc)
        rt_kprintf("Bluetooth HID obsolete bond cleanup failed\n");
}

static int load_bonds(struct classic_hid_bonds *out)
{
    uint32_t base;
    if (!out) return -RT_EINVAL;
#if (CY_CPU_CORTEX_M55)
    int setup = prepare_rram_region();
    if (setup) return setup;
#endif
    int rc = locate_journal(BONDS_AREA_OFFSET, BONDS_COPY_STRIDE, bonds_owner, false, &base);
    if (rc == -RT_EEMPTY) return load_legacy(out);
    if (rc) return rc;

    int active;
    struct commit record;
    rc = scan_bonds(base, &active, &record);
    if (rc) return rc;
    if (active < 0) return load_legacy(out);
    if (read_bytes(base + active * BONDS_COPY_STRIDE + sizeof(record), bonds_payload, record.size) ||
        checksum_data(&record, bonds_payload) != record.crc) return -RT_EIO;
    decode_bonds(out);
    scrub_obsolete_bonds(base + (1 - active) * BONDS_COPY_STRIDE);
    return out->count ? 0 : -RT_EEMPTY;
}

static int save_bonds(const struct classic_hid_bonds *records)
{
    if (!records || records->count > CLASSIC_HID_MAX_BONDS) return -RT_EINVAL;
#if (CY_CPU_CORTEX_M55)
    int setup = prepare_rram_region();
    if (setup) return setup;
#endif
    uint32_t base;
    int rc = locate_journal(BONDS_AREA_OFFSET, BONDS_COPY_STRIDE, bonds_owner, true, &base);
    if (rc) return rc;

    int active;
    struct commit last;
    rc = scan_bonds(base, &active, &last);
    if (rc) return rc;
    unsigned target = active == 0 ? 1u : 0u;
    uint32_t addr = base + target * BONDS_COPY_STRIDE;
    struct commit next = {BONDS_COMMIT_TAG, active < 0 ? 1u : last.sequence + 1u, BONDS_DATA_SIZE, 0};
    struct commit check;
    encode_bonds(records);
    next.crc = checksum_data(&next, bonds_payload);
    if (write_bytes(addr, zeros, sizeof(next)) ||
        write_bytes(addr + sizeof(next), bonds_payload, sizeof(bonds_payload)) ||
        read_bytes(addr + sizeof(next), bonds_payload, sizeof(bonds_payload)) ||
        checksum_data(&next, bonds_payload) != next.crc ||
        write_bytes(addr, &next, sizeof(next)) || read_bytes(addr, &check, sizeof(check)) ||
        memcmp(&next, &check, sizeof(next))) return -RT_EIO;
    scrub_obsolete_bonds(base + (1u - target) * BONDS_COPY_STRIDE);
    return 0;
}

const struct classic_hid_store edgi_classic_hid_store = {
    .load = load_bonds, .save = save_bonds
};
