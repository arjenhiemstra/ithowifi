// Phase 2 native unit tests: RF-related pure logic
// Tests end byte selection, command name lookup, and RF device management

#include <unity.h>
#include <cstdint>
#include <cstring>

// ---------------------------------------------------------------------------
// End byte selection logic (from IthoCC1101.cpp)
// Even decoded length -> 0xAC, odd -> 0xCA
// ---------------------------------------------------------------------------

uint8_t getEndByte(uint8_t packetLength) {
    return (packetLength % 2 == 0) ? 0xAC : 0xCA;
}

// ---------------------------------------------------------------------------
// Command name lookup (from IthoRemote.cpp remote_command_msg_table)
// ---------------------------------------------------------------------------

enum IthoCommand {
    IthoUnknown = 0,
    IthoJoin = 1,
    IthoLeave = 2,
    IthoAway = 3,
    IthoLow = 4,
    IthoMedium = 5,
    IthoHigh = 6,
    IthoFull = 7,
    IthoTimer1 = 8,
    IthoTimer2 = 9,
    IthoTimer3 = 10,
    IthoAuto = 11,
    IthoAutoNight = 12,
    IthoCook30 = 13,
    IthoCook60 = 14,
    IthoTimerUser = 15,
    IthoJoinReply = 16,
    IthoPIRmotionOn = 17,
    IthoPIRmotionOff = 18,
    Itho31D9 = 19,
    Itho31DA = 20,
    IthoDeviceInfo = 21
};

struct CommandEntry {
    uint8_t code;
    const char *msg;
};

static const CommandEntry command_table[] = {
    {IthoUnknown, "IthoUnknown"},
    {IthoJoin, "IthoJoin"},
    {IthoLeave, "IthoLeave"},
    {IthoAway, "IthoAway"},
    {IthoLow, "IthoLow"},
    {IthoMedium, "IthoMedium"},
    {IthoHigh, "IthoHigh"},
    {IthoFull, "IthoFull"},
    {IthoTimer1, "IthoTimer1"},
    {IthoTimer2, "IthoTimer2"},
    {IthoTimer3, "IthoTimer3"},
    {IthoAuto, "IthoAuto"},
    {IthoAutoNight, "IthoAutoNight"},
    {IthoCook30, "IthoCook30"},
    {IthoCook60, "IthoCook60"},
    {IthoTimerUser, "IthoTimerUser"},
    {IthoJoinReply, "IthoJoinReply"},
    {IthoPIRmotionOn, "IthoPIRmotionOn"},
    {IthoPIRmotionOff, "IthoPIRmotionOff"},
    {Itho31D9, "Itho31D9"},
    {Itho31DA, "Itho31DA"},
    {IthoDeviceInfo, "IthoDeviceInfo"},
};

static const char *unknown_msg = "CMD UNKNOWN ERROR";

const char *rem_cmd_to_name(uint8_t code) {
    for (size_t i = 0; i < sizeof(command_table) / sizeof(command_table[0]); ++i) {
        if (command_table[i].code == code) {
            return command_table[i].msg;
        }
    }
    return unknown_msg;
}

// ---------------------------------------------------------------------------
// RF device management (array-based tracking, from IthoRemote logic)
// ---------------------------------------------------------------------------

#define MAX_RF_DEVICES 8

struct RFDevice {
    uint8_t ID[3]{0, 0, 0};
};

static RFDevice rfDevices[MAX_RF_DEVICES];

void resetRFDevices() {
    for (int i = 0; i < MAX_RF_DEVICES; i++) {
        rfDevices[i].ID[0] = 0;
        rfDevices[i].ID[1] = 0;
        rfDevices[i].ID[2] = 0;
    }
}

bool isEmptyRFSlot(int index) {
    if (index < 0 || index >= MAX_RF_DEVICES) return true;
    return (rfDevices[index].ID[0] == 0 &&
            rfDevices[index].ID[1] == 0 &&
            rfDevices[index].ID[2] == 0);
}

int getRemoteIndexByID(uint8_t b0, uint8_t b1, uint8_t b2) {
    for (int i = 0; i < MAX_RF_DEVICES; i++) {
        if (rfDevices[i].ID[0] == b0 &&
            rfDevices[i].ID[1] == b1 &&
            rfDevices[i].ID[2] == b2)
            return i;
    }
    return -1;
}

bool checkRFDevice(uint8_t b0, uint8_t b1, uint8_t b2) {
    if (b0 == 0 && b1 == 0 && b2 == 0) return false;
    return getRemoteIndexByID(b0, b1, b2) >= 0;
}

// Returns index on success, -1 if duplicate, -2 if full
int addRFDevice(uint8_t b0, uint8_t b1, uint8_t b2) {
    if (checkRFDevice(b0, b1, b2)) return -1;
    int idx = getRemoteIndexByID(0, 0, 0);
    if (idx < 0) return -2;
    rfDevices[idx].ID[0] = b0;
    rfDevices[idx].ID[1] = b1;
    rfDevices[idx].ID[2] = b2;
    return idx;
}

int removeRFDevice(uint8_t b0, uint8_t b1, uint8_t b2) {
    int idx = getRemoteIndexByID(b0, b1, b2);
    if (idx < 0) return -1;
    rfDevices[idx].ID[0] = 0;
    rfDevices[idx].ID[1] = 0;
    rfDevices[idx].ID[2] = 0;
    return idx;
}

// ---------------------------------------------------------------------------
// End byte tests
// ---------------------------------------------------------------------------

void test_end_byte_even_12(void) {
    TEST_ASSERT_EQUAL_HEX8(0xAC, getEndByte(12));
}

void test_end_byte_even_14(void) {
    TEST_ASSERT_EQUAL_HEX8(0xAC, getEndByte(14));
}

void test_end_byte_odd_15(void) {
    TEST_ASSERT_EQUAL_HEX8(0xCA, getEndByte(15));
}

void test_end_byte_odd_23(void) {
    TEST_ASSERT_EQUAL_HEX8(0xCA, getEndByte(23));
}

void test_end_byte_odd_29(void) {
    TEST_ASSERT_EQUAL_HEX8(0xCA, getEndByte(29));
}

void test_end_byte_odd_49(void) {
    TEST_ASSERT_EQUAL_HEX8(0xCA, getEndByte(49));
}

void test_end_byte_even_0(void) {
    TEST_ASSERT_EQUAL_HEX8(0xAC, getEndByte(0));
}

void test_end_byte_odd_1(void) {
    TEST_ASSERT_EQUAL_HEX8(0xCA, getEndByte(1));
}

// ---------------------------------------------------------------------------
// Command name lookup tests
// ---------------------------------------------------------------------------

void test_cmd_name_unknown(void) {
    TEST_ASSERT_EQUAL_STRING("IthoUnknown", rem_cmd_to_name(IthoUnknown));
}

void test_cmd_name_join(void) {
    TEST_ASSERT_EQUAL_STRING("IthoJoin", rem_cmd_to_name(IthoJoin));
}

void test_cmd_name_leave(void) {
    TEST_ASSERT_EQUAL_STRING("IthoLeave", rem_cmd_to_name(IthoLeave));
}

void test_cmd_name_low(void) {
    TEST_ASSERT_EQUAL_STRING("IthoLow", rem_cmd_to_name(IthoLow));
}

void test_cmd_name_medium(void) {
    TEST_ASSERT_EQUAL_STRING("IthoMedium", rem_cmd_to_name(IthoMedium));
}

void test_cmd_name_high(void) {
    TEST_ASSERT_EQUAL_STRING("IthoHigh", rem_cmd_to_name(IthoHigh));
}

void test_cmd_name_full(void) {
    TEST_ASSERT_EQUAL_STRING("IthoFull", rem_cmd_to_name(IthoFull));
}

void test_cmd_name_auto(void) {
    TEST_ASSERT_EQUAL_STRING("IthoAuto", rem_cmd_to_name(IthoAuto));
}

void test_cmd_name_device_info(void) {
    TEST_ASSERT_EQUAL_STRING("IthoDeviceInfo", rem_cmd_to_name(IthoDeviceInfo));
}

void test_cmd_name_invalid_returns_unknown(void) {
    TEST_ASSERT_EQUAL_STRING("CMD UNKNOWN ERROR", rem_cmd_to_name(255));
}

// ---------------------------------------------------------------------------
// RF device management tests
// ---------------------------------------------------------------------------

void test_rf_add_find_remove_cycle(void) {
    resetRFDevices();
    int idx = addRFDevice(0xAA, 0xBB, 0xCC);
    TEST_ASSERT_EQUAL_INT(0, idx);
    TEST_ASSERT_TRUE(checkRFDevice(0xAA, 0xBB, 0xCC));
    TEST_ASSERT_EQUAL_INT(0, getRemoteIndexByID(0xAA, 0xBB, 0xCC));

    int rem = removeRFDevice(0xAA, 0xBB, 0xCC);
    TEST_ASSERT_EQUAL_INT(0, rem);
    TEST_ASSERT_FALSE(checkRFDevice(0xAA, 0xBB, 0xCC));
}

void test_rf_duplicate_detection(void) {
    resetRFDevices();
    addRFDevice(0x11, 0x22, 0x33);
    int result = addRFDevice(0x11, 0x22, 0x33);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_rf_full_array(void) {
    resetRFDevices();
    for (int i = 0; i < MAX_RF_DEVICES; i++) {
        int result = addRFDevice(0x10 + i, 0x20 + i, 0x30 + i);
        TEST_ASSERT_TRUE(result >= 0);
    }
    int result = addRFDevice(0xFF, 0xFE, 0xFD);
    TEST_ASSERT_EQUAL_INT(-2, result);
}

void test_rf_boundary_indices(void) {
    resetRFDevices();
    // First slot
    int idx0 = addRFDevice(0x01, 0x01, 0x01);
    TEST_ASSERT_EQUAL_INT(0, idx0);

    // Fill remaining slots to test last slot
    for (int i = 1; i < MAX_RF_DEVICES; i++) {
        addRFDevice(0x01, 0x01, 0x01 + i);
    }
    // Last device should be at index MAX_RF_DEVICES-1
    int lastIdx = getRemoteIndexByID(0x01, 0x01, 0x01 + MAX_RF_DEVICES - 1);
    TEST_ASSERT_EQUAL_INT(MAX_RF_DEVICES - 1, lastIdx);
}

void test_rf_remove_nonexistent(void) {
    resetRFDevices();
    int result = removeRFDevice(0xDE, 0xAD, 0x00);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_rf_check_zero_id_returns_false(void) {
    resetRFDevices();
    TEST_ASSERT_FALSE(checkRFDevice(0, 0, 0));
}

void test_rf_empty_slot_check(void) {
    resetRFDevices();
    TEST_ASSERT_TRUE(isEmptyRFSlot(0));
    addRFDevice(0xAA, 0xBB, 0xCC);
    TEST_ASSERT_FALSE(isEmptyRFSlot(0));
}

void test_rf_empty_slot_out_of_range(void) {
    resetRFDevices();
    TEST_ASSERT_TRUE(isEmptyRFSlot(-1));
    TEST_ASSERT_TRUE(isEmptyRFSlot(MAX_RF_DEVICES));
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // End byte
    RUN_TEST(test_end_byte_even_12);
    RUN_TEST(test_end_byte_even_14);
    RUN_TEST(test_end_byte_odd_15);
    RUN_TEST(test_end_byte_odd_23);
    RUN_TEST(test_end_byte_odd_29);
    RUN_TEST(test_end_byte_odd_49);
    RUN_TEST(test_end_byte_even_0);
    RUN_TEST(test_end_byte_odd_1);

    // Command name lookup
    RUN_TEST(test_cmd_name_unknown);
    RUN_TEST(test_cmd_name_join);
    RUN_TEST(test_cmd_name_leave);
    RUN_TEST(test_cmd_name_low);
    RUN_TEST(test_cmd_name_medium);
    RUN_TEST(test_cmd_name_high);
    RUN_TEST(test_cmd_name_full);
    RUN_TEST(test_cmd_name_auto);
    RUN_TEST(test_cmd_name_device_info);
    RUN_TEST(test_cmd_name_invalid_returns_unknown);

    // RF device management
    RUN_TEST(test_rf_add_find_remove_cycle);
    RUN_TEST(test_rf_duplicate_detection);
    RUN_TEST(test_rf_full_array);
    RUN_TEST(test_rf_boundary_indices);
    RUN_TEST(test_rf_remove_nonexistent);
    RUN_TEST(test_rf_check_zero_id_returns_false);
    RUN_TEST(test_rf_empty_slot_check);
    RUN_TEST(test_rf_empty_slot_out_of_range);

    return UNITY_END();
}
