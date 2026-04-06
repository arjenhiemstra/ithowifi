// Phase 2 native unit tests: IthoQueue logic
// Tests queue add, sort, timer, clear, fallback, and JSON serialization

#include <unity.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <cstdint>

// ---------------------------------------------------------------------------
// Reimplemented IthoQueue logic (from IthoQueue.h / IthoQueue.cpp)
// ---------------------------------------------------------------------------

#define MAX_QUEUE 10
#define QUEUE_UPDATE_MS 100

class IthoQueue {
private:
    struct Queue {
        int16_t speed{-1};
        unsigned long valid{0};
        void get(JsonObject obj, int index) const {
            obj["index"] = index;
            obj["speed"] = speed;
            obj["valid"] = valid;
        }
    };

    struct Queue items[MAX_QUEUE];

public:
    uint16_t ithoSpeed = 0;
    uint16_t ithoOldSpeed = 0;
    uint16_t fallBackSpeed = 42;
    bool ithoSpeedUpdated = false;

    IthoQueue() {}
    ~IthoQueue() {}

    bool addToQueue(int speedVal, unsigned long validVal, uint8_t nonQ_cmd_clearsQ) {
        if (validVal == 0) {
            fallBackSpeed = speedVal;
            if (nonQ_cmd_clearsQ) {
                clearQueue();
            }
            return true;
        } else {
            validVal = validVal * 60 * 1000; // minutes to milliseconds
            for (int i = 0; i < MAX_QUEUE; i++) {
                if (items[i].speed == -1) {
                    items[i].speed = speedVal;
                    items[i].valid = validVal;
                    return true;
                }
            }
            std::sort(items, items + MAX_QUEUE, [](const Queue s1, const Queue s2) {
                return (s1.speed > s2.speed);
            });
            if (items[MAX_QUEUE - 1].speed < speedVal) {
                items[MAX_QUEUE - 1].speed = speedVal;
                items[MAX_QUEUE - 1].valid = validVal;
                return true;
            }
            return false;
        }
    }

    void updateQueue() {
        for (int i = 0; i < MAX_QUEUE; i++) {
            if (items[i].valid > 0) {
                items[i].valid -= QUEUE_UPDATE_MS;
            }
            if (items[i].valid == 0) {
                items[i].speed = -1;
            }
        }
        std::sort(items, items + MAX_QUEUE, [](const Queue s1, const Queue s2) {
            return (s1.speed > s2.speed);
        });
        ithoOldSpeed = ithoSpeed;
        if (items[0].speed == -1) {
            ithoSpeed = fallBackSpeed;
        } else {
            ithoSpeed = items[0].speed;
        }
        if (ithoOldSpeed != ithoSpeed) {
            ithoSpeedUpdated = true;
        }
    }

    void clearQueue() {
        for (int i = 0; i < MAX_QUEUE; i++) {
            items[i].speed = -1;
            items[i].valid = 0;
        }
    }

    void get(JsonArray arr) {
        for (int i = 0; i < MAX_QUEUE; i++) {
            items[i].get(arr.add<JsonObject>(), i);
        }
    }

    // Test helpers to inspect internal state
    int16_t getItemSpeed(int index) const { return items[index].speed; }
    unsigned long getItemValid(int index) const { return items[index].valid; }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_add_single_item(void) {
    IthoQueue q;
    // validVal=1 means 1 minute = 60000ms
    bool ok = q.addToQueue(100, 1, 0);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(100, q.getItemSpeed(0));
    TEST_ASSERT_EQUAL(60000UL, q.getItemValid(0));
}

void test_add_timed_item_timer_decrements(void) {
    IthoQueue q;
    q.addToQueue(80, 1, 0); // 1 min = 60000ms
    q.updateQueue(); // decrements by QUEUE_UPDATE_MS (100)
    TEST_ASSERT_EQUAL(59900UL, q.getItemValid(0));
    TEST_ASSERT_EQUAL_INT(80, q.ithoSpeed);
}

void test_add_multiple_sorted_by_speed(void) {
    IthoQueue q;
    q.addToQueue(50, 1, 0);
    q.addToQueue(200, 1, 0);
    q.addToQueue(100, 1, 0);
    // After updateQueue, items are sorted highest first
    q.updateQueue();
    TEST_ASSERT_EQUAL_INT(200, q.ithoSpeed);
}

void test_clear_queue(void) {
    IthoQueue q;
    q.addToQueue(100, 1, 0);
    q.addToQueue(200, 1, 0);
    q.clearQueue();
    // All items should be -1
    for (int i = 0; i < MAX_QUEUE; i++) {
        TEST_ASSERT_EQUAL_INT(-1, q.getItemSpeed(i));
        TEST_ASSERT_EQUAL(0UL, q.getItemValid(i));
    }
}

void test_queue_json_serialization(void) {
    IthoQueue q;
    q.addToQueue(150, 2, 0); // 2 min = 120000ms

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    q.get(arr);

    TEST_ASSERT_EQUAL_INT(MAX_QUEUE, (int)arr.size());
    // First item should have our values
    TEST_ASSERT_EQUAL_INT(0, arr[0]["index"].as<int>());
    TEST_ASSERT_EQUAL_INT(150, arr[0]["speed"].as<int>());
    TEST_ASSERT_EQUAL(120000UL, arr[0]["valid"].as<unsigned long>());
    // Second item should be empty (-1 speed)
    TEST_ASSERT_EQUAL_INT(-1, arr[1]["speed"].as<int>());
}

void test_fallback_speed_when_queue_empty(void) {
    IthoQueue q;
    q.fallBackSpeed = 42;
    // Don't add any timed items, just update
    q.updateQueue();
    TEST_ASSERT_EQUAL_INT(42, q.ithoSpeed);
}

void test_fallback_speed_set_by_zero_valid(void) {
    IthoQueue q;
    // validVal=0 means set fallback speed
    q.addToQueue(77, 0, 0);
    TEST_ASSERT_EQUAL_INT(77, q.fallBackSpeed);
    q.updateQueue();
    // With no timed items, speed should be fallback
    TEST_ASSERT_EQUAL_INT(77, q.ithoSpeed);
}

void test_zero_valid_with_clear_flag(void) {
    IthoQueue q;
    q.addToQueue(100, 1, 0);
    // Setting fallback with nonQ_cmd_clearsQ=1 should also clear queue
    q.addToQueue(50, 0, 1);
    TEST_ASSERT_EQUAL_INT(50, q.fallBackSpeed);
    // Queue should be cleared
    TEST_ASSERT_EQUAL_INT(-1, q.getItemSpeed(0));
}

void test_item_expires_after_enough_updates(void) {
    IthoQueue q;
    q.fallBackSpeed = 10;
    // Add item with 1 "minute" = 60000ms, each update removes 100ms
    // Set valid directly small for testing: use raw add
    q.addToQueue(200, 1, 0); // 60000ms

    // Simulate 600 updates (600 * 100ms = 60000ms = expires)
    for (int i = 0; i < 600; i++) {
        q.updateQueue();
    }
    // Item should have expired, speed should be fallback
    TEST_ASSERT_EQUAL_INT(10, q.ithoSpeed);
}

void test_speed_updated_flag(void) {
    IthoQueue q;
    q.ithoSpeed = 42;
    q.ithoOldSpeed = 42;
    q.ithoSpeedUpdated = false;
    q.addToQueue(200, 1, 0);
    q.updateQueue();
    // Speed changed from 42 to 200
    TEST_ASSERT_TRUE(q.ithoSpeedUpdated);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_add_single_item);
    RUN_TEST(test_add_timed_item_timer_decrements);
    RUN_TEST(test_add_multiple_sorted_by_speed);
    RUN_TEST(test_clear_queue);
    RUN_TEST(test_queue_json_serialization);
    RUN_TEST(test_fallback_speed_when_queue_empty);
    RUN_TEST(test_fallback_speed_set_by_zero_valid);
    RUN_TEST(test_zero_valid_with_clear_flag);
    RUN_TEST(test_item_expires_after_enough_updates);
    RUN_TEST(test_speed_updated_flag);

    return UNITY_END();
}
