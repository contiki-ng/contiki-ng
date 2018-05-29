import unittest, array, time

class TestQueueModeAwake(unittest.TestCase):
    global respAwakeTime
    global respSleepTime

    def test_read_awake_time(self):
        self.assertEqual(respAwakeTime.getCode().getName(), "CONTENT")

    def test_read_sleep_time(self):
        self.assertEqual(respSleepTime.getCode().getName(), "CONTENT")


print "----------------------------------------"
print "LWM2M Queue Mode Awake State Tester"
print "----------------------------------------"


suite = unittest.TestLoader().loadTestsFromTestCase(TestQueueModeAwake)
unittest.TextTestRunner(verbosity=2).run(suite)
