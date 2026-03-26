// Phase 2 native unit tests: IthoRemote slot management logic
// Tests isEmptySlot, getRemoteCount, remoteIndex, checkID, registerNewRemote

#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// Reimplemented IthoRemote slot-management logic (from IthoRemote.h/.cpp)
// Stripped of all hardware dependencies (no Ticker, no sys, no IthoPacket)
// ---------------------------------------------------------------------------

#define MAX_NUM_OF_REMOTES 12

class TestIthoRemote {
private:
    struct Remote {
        uint8_t ID[3]{0, 0, 0};
        char name[32];
    };

    Remote remotes[MAX_NUM_OF_REMOTES];
    int maxRemotes{MAX_NUM_OF_REMOTES};

public:
    TestIthoRemote() {
        for (int i = 0; i < MAX_NUM_OF_REMOTES; i++) {
            snprintf(remotes[i].name, sizeof(remotes[i].name), "remote%d", i);
            remotes[i].ID[0] = 0;
            remotes[i].ID[1] = 0;
            remotes[i].ID[2] = 0;
        }
    }

    bool isEmptySlot(int index) const {
        if (index < 0 || index >= maxRemotes)
            return true;
        return (remotes[index].ID[0] == 0 &&
                remotes[index].ID[1] == 0 &&
                remotes[index].ID[2] == 0);
    }

    int getRemoteCount() const {
        int count = 0;
        for (int i = 0; i < maxRemotes; i++) {
            if (!isEmptySlot(i))
                count++;
        }
        return count;
    }

    int remoteIndex(uint8_t byte0, uint8_t byte1, uint8_t byte2) const {
        for (int i = 0; i < maxRemotes; i++) {
            if (byte0 == remotes[i].ID[0] &&
                byte1 == remotes[i].ID[1] &&
                byte2 == remotes[i].ID[2])
                return i;
        }
        return -1;
    }

    bool checkID(uint8_t byte0, uint8_t byte1, uint8_t byte2) const {
        if (byte0 == 0 && byte1 == 0 && byte2 == 0)
            return false;
        for (int i = 0; i < maxRemotes; i++) {
            if (byte0 == remotes[i].ID[0] &&
                byte1 == remotes[i].ID[1] &&
                byte2 == remotes[i].ID[2])
                return true;
        }
        return false;
    }

    // Returns index on success, -1 if duplicate, -2 if full
    int registerNewRemote(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
        if (checkID(byte0, byte1, byte2)) {
            return -1; // already registered
        }
        int index = remoteIndex(0, 0, 0); // find first empty slot
        if (index < 0) {
            return -2; // no free slot
        }
        remotes[index].ID[0] = byte0;
        remotes[index].ID[1] = byte1;
        remotes[index].ID[2] = byte2;
        return index;
    }

    // Remove by zeroing ID bytes
    int removeRemote(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
        if (!checkID(byte0, byte1, byte2)) {
            return -1;
        }
        int index = remoteIndex(byte0, byte1, byte2);
        if (index < 0) return -1;
        remotes[index].ID[0] = 0;
        remotes[index].ID[1] = 0;
        remotes[index].ID[2] = 0;
        snprintf(remotes[index].name, sizeof(remotes[index].name), "remote%d", index);
        return 1;
    }

    int getMaxRemotes() const { return maxRemotes; }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_empty_remote_array_count_zero(void) {
    TestIthoRemote r;
    TEST_ASSERT_EQUAL_INT(0, r.getRemoteCount());
}

void test_register_one_remote(void) {
    TestIthoRemote r;
    int idx = r.registerNewRemote(0xAA, 0xBB, 0xCC);
    TEST_ASSERT_EQUAL_INT(0, idx); // first empty slot
    TEST_ASSERT_EQUAL_INT(1, r.getRemoteCount());
}

void test_registered_remote_findable_by_id(void) {
    TestIthoRemote r;
    r.registerNewRemote(0xAA, 0xBB, 0xCC);
    int idx = r.remoteIndex(0xAA, 0xBB, 0xCC);
    TEST_ASSERT_EQUAL_INT(0, idx);
    TEST_ASSERT_TRUE(r.checkID(0xAA, 0xBB, 0xCC));
}

void test_register_multiple_remotes(void) {
    TestIthoRemote r;
    r.registerNewRemote(0x01, 0x02, 0x03);
    r.registerNewRemote(0x04, 0x05, 0x06);
    r.registerNewRemote(0x07, 0x08, 0x09);
    TEST_ASSERT_EQUAL_INT(3, r.getRemoteCount());
}

void test_register_duplicate_returns_neg1(void) {
    TestIthoRemote r;
    r.registerNewRemote(0xAA, 0xBB, 0xCC);
    int result = r.registerNewRemote(0xAA, 0xBB, 0xCC);
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_EQUAL_INT(1, r.getRemoteCount());
}

void test_register_full_array_returns_neg2(void) {
    TestIthoRemote r;
    for (int i = 0; i < MAX_NUM_OF_REMOTES; i++) {
        int result = r.registerNewRemote(0x10 + i, 0x20 + i, 0x30 + i);
        TEST_ASSERT_TRUE(result >= 0);
    }
    TEST_ASSERT_EQUAL_INT(MAX_NUM_OF_REMOTES, r.getRemoteCount());
    // One more should fail
    int result = r.registerNewRemote(0xFF, 0xFE, 0xFD);
    TEST_ASSERT_EQUAL_INT(-2, result);
}

void test_remove_remote_decreases_count(void) {
    TestIthoRemote r;
    r.registerNewRemote(0xAA, 0xBB, 0xCC);
    r.registerNewRemote(0x11, 0x22, 0x33);
    TEST_ASSERT_EQUAL_INT(2, r.getRemoteCount());

    int result = r.removeRemote(0xAA, 0xBB, 0xCC);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_INT(1, r.getRemoteCount());
    TEST_ASSERT_FALSE(r.checkID(0xAA, 0xBB, 0xCC));
}

void test_is_empty_slot_zero_id(void) {
    TestIthoRemote r;
    TEST_ASSERT_TRUE(r.isEmptySlot(0));
}

void test_is_empty_slot_nonzero_id(void) {
    TestIthoRemote r;
    r.registerNewRemote(0x01, 0x02, 0x03);
    TEST_ASSERT_FALSE(r.isEmptySlot(0));
}

void test_is_empty_slot_out_of_range(void) {
    TestIthoRemote r;
    TEST_ASSERT_TRUE(r.isEmptySlot(-1));
    TEST_ASSERT_TRUE(r.isEmptySlot(MAX_NUM_OF_REMOTES));
    TEST_ASSERT_TRUE(r.isEmptySlot(999));
}

void test_checkID_zero_returns_false(void) {
    TestIthoRemote r;
    TEST_ASSERT_FALSE(r.checkID(0, 0, 0));
}

void test_checkID_unregistered_returns_false(void) {
    TestIthoRemote r;
    TEST_ASSERT_FALSE(r.checkID(0xDE, 0xAD, 0x00));
}

void test_remove_nonexistent_returns_neg1(void) {
    TestIthoRemote r;
    int result = r.removeRemote(0xDE, 0xAD, 0x00);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_register_after_remove_reuses_slot(void) {
    TestIthoRemote r;
    r.registerNewRemote(0xAA, 0xBB, 0xCC);
    r.registerNewRemote(0x11, 0x22, 0x33);
    r.removeRemote(0xAA, 0xBB, 0xCC); // frees slot 0
    int idx = r.registerNewRemote(0xDD, 0xEE, 0xFF);
    TEST_ASSERT_EQUAL_INT(0, idx); // reuses slot 0
    TEST_ASSERT_EQUAL_INT(2, r.getRemoteCount());
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_empty_remote_array_count_zero);
    RUN_TEST(test_register_one_remote);
    RUN_TEST(test_registered_remote_findable_by_id);
    RUN_TEST(test_register_multiple_remotes);
    RUN_TEST(test_register_duplicate_returns_neg1);
    RUN_TEST(test_register_full_array_returns_neg2);
    RUN_TEST(test_remove_remote_decreases_count);
    RUN_TEST(test_is_empty_slot_zero_id);
    RUN_TEST(test_is_empty_slot_nonzero_id);
    RUN_TEST(test_is_empty_slot_out_of_range);
    RUN_TEST(test_checkID_zero_returns_false);
    RUN_TEST(test_checkID_unregistered_returns_false);
    RUN_TEST(test_remove_nonexistent_returns_neg1);
    RUN_TEST(test_register_after_remove_reuses_slot);

    return UNITY_END();
}
