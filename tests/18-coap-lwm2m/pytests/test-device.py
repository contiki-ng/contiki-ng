import unittest, array, time

class TestDevice(unittest.TestCase):
    global client

    def test_available_power_sources(self):
        r = client.readTLV("3/0/6")
        self.assertEqual(r.getCode().getName(), "CONTENT")

    def test_device_read_JSON(self):
        r = client.readJSON("3/0/1")
        self.assertEqual(r.getCode().getName(), "CONTENT")

    def test_manufacturer_read(self):
        r = client.read("3/0/0")
        self.assertEqual(r.getCode().getName(), "CONTENT")

    def test_manufacturer_readJSON(self):
        r = client.readJSON("3/0/0")
        self.assertEqual(r.getCode().getName(), "CONTENT")

    def test_manufacturer_write(self):
        r = client.write(3, 0, 0, "abc");
        self.assertEqual(r.getCode().getName(), "METHOD_NOT_ALLOWED")

    def test_manufacturer_execute(self):
        r = client.execute("3/0/0")
        self.assertEqual(r.getCode().getName(), "METHOD_NOT_ALLOWED")

    def test_reboot_read(self):
        r = client.read("3/0/4")
        self.assertEqual(r.getCode().getName(), "METHOD_NOT_ALLOWED")

#    def test_opaque_read(self):
#        r = client.readTLV("4711/0/11000")
#        v = r.getContent().getValue();
        #print "Result:", v
        #print "Type: ", type(v)
        #print "Type code: ", v.typecode
        #print "Data size: ", len(v)
#        self.assertEqual(len(v), 900)

#    def test_object_with_opaque_read(self):
#        r = client.readTLV("4711/0/")
#        self.assertEqual(r.getCode().getName(), "CONTENT")

    def test_device_time_write(self):
        r = client.write(3,0,13,1000)
        self.assertEqual(r.getCode().getName(), "CHANGED")
        time.sleep(4.9)
        r = client.read("3/0/13")
        v = r.getContent().getValue().getTime()
        self.assertTrue(v > 1000)
        print "Time: ", v



print "----------------------------------------"
print "LWM2M Tester - name of client: ", client.endpoint
print "----------------------------------------"

r = client.read("3/0/0");
print "Code:", r.getCode(), r.getCode().getName() == "CONTENT"
print "Objects: ", client.links
print "Read  Manufacturer => ", client.read("3/0/0")
print "Read  Device => ", client.readTLV("3/0/")

suite = unittest.TestLoader().loadTestsFromTestCase(TestDevice)
unittest.TextTestRunner(verbosity=2).run(suite)
