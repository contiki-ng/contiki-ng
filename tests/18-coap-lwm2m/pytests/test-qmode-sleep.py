from __future__ import with_statement
import unittest, array, time

class TestQueueModeSleep(unittest.TestCase):
    global client

    def test_read_awake_time(self):
	self.assertIsNone(client.read("30000/0/30000"))

    def test_read_sleep_time(self):
	self.assertIsNone(client.read("30000/0/30001"))


print "----------------------------------------"
print "LWM2M Queue Mode Sleep State Tester - name of client: ", client.endpoint
print "----------------------------------------"

suite = unittest.TestLoader().loadTestsFromTestCase(TestQueueModeSleep)
unittest.TextTestRunner(verbosity=2).run(suite)
