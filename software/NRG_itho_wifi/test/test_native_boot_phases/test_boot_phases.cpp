// Boot-phase dependency-graph invariants (ADR-0010).
//
// This mirrors, by hand, the phase bits in main/tasks/boot_phases.h and the
// per-task wait masks in the six task_*.cpp files. It does not read the firmware
// source; it guards the *design* against deadlock/cycle/unset-wait regressions.
// Keep the tables below in sync when the boot phases change — the adversarial
// review verifies the actual code matches this model.

#include <unity.h>
#include <cstdint>

// --- mirror of boot_phases.h ---
enum : uint32_t
{
  HW = 1u << 0,
  CONFIG = 1u << 1,
  NET = 1u << 2,
  RF = 1u << 3,
  WEB = 1u << 4,
};

// Each phase and the phases that must already be set before its producing task
// can set it (i.e. that producer task's wait mask).
struct Phase
{
  const char *name;
  uint32_t bit;
  uint32_t deps;
};
static const Phase phases[] = {
    {"HW", HW, 0},                // TaskInit  (sets HW first, then waits WEB)
    {"CONFIG", CONFIG, HW},       // TaskConfigAndLog
    {"NET", NET, CONFIG},         // TaskSysControl
    {"RF", RF, CONFIG},           // TaskCC1101
    {"WEB", WEB, CONFIG | NET},   // TaskWeb (= boot complete)
};
static const int NP = sizeof(phases) / sizeof(phases[0]);

// The wait mask each task blocks on before its dependent init.
static const uint32_t taskWaits[] = {
    WEB,          // TaskInit (terminal boot-complete barrier)
    HW,           // TaskConfigAndLog
    CONFIG,       // TaskSysControl
    CONFIG,       // TaskCC1101
    CONFIG | NET, // TaskMQTT
    CONFIG | NET, // TaskWeb
};
static const int NT = sizeof(taskWaits) / sizeof(taskWaits[0]);

// A Kahn-style fixpoint: repeatedly set any phase whose deps are already set.
// If every phase becomes set, the graph is acyclic and boot cannot deadlock.
void test_phase_graph_is_live_and_acyclic(void)
{
  uint32_t set = 0, all = 0;
  for (int i = 0; i < NP; i++)
    all |= phases[i].bit;

  bool progress = true;
  while (progress)
  {
    progress = false;
    for (int i = 0; i < NP; i++)
      if (!(set & phases[i].bit) && (set & phases[i].deps) == phases[i].deps)
      {
        set |= phases[i].bit;
        progress = true;
      }
  }
  TEST_ASSERT_EQUAL_HEX32(all, set); // all phases reachable => no cycle / no deadlock
}

void test_no_phase_depends_on_itself(void)
{
  for (int i = 0; i < NP; i++)
    TEST_ASSERT_EQUAL_HEX32(0, phases[i].deps & phases[i].bit);
}

// No task may wait on a bit that no task ever sets (that would hang forever).
void test_every_waited_bit_is_produced(void)
{
  uint32_t produced = 0;
  for (int i = 0; i < NP; i++)
    produced |= phases[i].bit;
  for (int i = 0; i < NT; i++)
    TEST_ASSERT_EQUAL_HEX32(taskWaits[i], taskWaits[i] & produced);
}

int main()
{
  UNITY_BEGIN();
  RUN_TEST(test_phase_graph_is_live_and_acyclic);
  RUN_TEST(test_no_phase_depends_on_itself);
  RUN_TEST(test_every_waited_bit_is_produced);
  return UNITY_END();
}
