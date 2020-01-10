import unittest
from unittest import mock

import collections

RetCode = collections.namedtuple('RetCode', ['returncode'])

FFInvoke = collections.namedtuple('FFInvoke', ['input', 'output', 'start', 'time', 'allargs'])

from process_lib import cut_on_edl

class EdlTesting(unittest.TestCase):

    def parse_ff_call(self, call):
        call = call.args[0]
        if call[0] != "ffmpeg":
            return FFInvoke(None, None, None, None, call)
        inp = call[call.index('-i')+1]
        out = call[len(call)-1]
        start = None
        time = None
        try:
            startIdx = call.index('-ss')
            start = float(call[startIdx+1])
        except ValueError:
            pass
        try:
            timeIdx = call.index('-t')
            time = float(call[timeIdx+1])
        except ValueError:
            pass

        return FFInvoke(inp, out, start, time, call)

    @mock.patch('subprocess.run')
    def test_straightforward(self, mock_subproc_run):
        mock_subproc_run.return_value = RetCode(0)
        ret = cut_on_edl("/fake_file.ts", ['735.28\t988.04\t0\n', '1430.91\t1706.92\t0\n', '3551.83\t3603.72\t0\n'], "/output.ts", "/tempdir")
        self.assertTrue(ret)
        ffcalls = [self.parse_ff_call(i) for i in mock_subproc_run.call_args_list]

        # We're making some assumptions about how cut_on_edl works. It's not really a black box test.
        # The main point of this test is to allow adjustments to the algorithm used to make the ffmpeg calls,
        # while ensuring that some cuts aren't missed or done incorrectly.

        # Start by checking that the cuts were made appropriately.
        cuttingcalls = [i for i in ffcalls if i.start != None and i.input == "/fake_file.ts"]
        cuttingcalls.sort(key=lambda x: float(x.start))
        
        # Three EDL entries not starting or ending the file will produce four segments
        self.assertEqual(len(cuttingcalls), 4)

        self.assertAlmostEqual(cuttingcalls[0].start, 0.0)
        self.assertAlmostEqual(cuttingcalls[0].time, 738.28, places=2) # note the three second buffer!

        self.assertAlmostEqual(cuttingcalls[1].start, 988.04, places=2)
        self.assertAlmostEqual(cuttingcalls[1].time, 445.87, places=2)

        self.assertAlmostEqual(cuttingcalls[2].start, 1706.92, places=2)
        self.assertAlmostEqual(cuttingcalls[2].time, 1847.91, places=2)

        self.assertAlmostEqual(cuttingcalls[3].start, 3603.72, places=2)
        self.assertEqual(cuttingcalls[3].time, None)

        # Ensure the tempdir was used for all cutting calls
        outputs=""
        for call in cuttingcalls:
            self.assertEqual(call.input, "/fake_file.ts")
            self.assertIn("/tempdir", call.output)
            outputs += call.output + "|"
        
        # Ensure the final ffmpeg call did the right thing, using the final output file, and input was concat.
        concatCall = [i for i in ffcalls if i.output == "/output.ts"]
        self.assertEqual(len(concatCall), 1)
        concatCall = concatCall[0]
        self.assertEqual(concatCall.input, "concat:"+outputs[:-1])

    @mock.patch('subprocess.run')
    def test_start_with_cut(self, mock_subproc_run):
        mock_subproc_run.return_value = RetCode(0)
        ret = cut_on_edl("/fake_file.ts", ['0.00\t988.04\t0\n', '1430.91\t1706.92\t0\n', '3551.83\t3603.72\t0\n'], "/output.ts", "/tempdir")
        self.assertTrue(ret)
        ffcalls = [self.parse_ff_call(i) for i in mock_subproc_run.call_args_list]

        # We're making some assumptions about how cut_on_edl works. It's not really a black box test.
        # The main point of this test is to allow adjustments to the algorithm used to make the ffmpeg calls,
        # while ensuring that some cuts aren't missed or done incorrectly.

        # Start by checking that the cuts were made appropriately.
        cuttingcalls = [i for i in ffcalls if i.start != None and i.input == "/fake_file.ts"]
        cuttingcalls.sort(key=lambda x: float(x.start))
        for call in cuttingcalls:
            print(call)
        
        # Three EDL entries, but with one starting the file will cause three files to be created.
        self.assertEqual(len(cuttingcalls), 3)

        self.assertAlmostEqual(cuttingcalls[0].start, 988.04, places=2)
        self.assertAlmostEqual(cuttingcalls[0].time, 445.87, places=2) # note the three second buffer!

        self.assertAlmostEqual(cuttingcalls[1].start, 1706.92, places=2)
        self.assertAlmostEqual(cuttingcalls[1].time, 1847.91, places=2)

        self.assertAlmostEqual(cuttingcalls[2].start, 3603.72, places=2)
        self.assertEqual(cuttingcalls[2].time, None)

        # Ensure the tempdir was used for all cutting calls
        outputs=""
        for call in cuttingcalls:
            self.assertEqual(call.input, "/fake_file.ts")
            self.assertIn("/tempdir", call.output)
            outputs += call.output + "|"
        
        # Ensure the final ffmpeg call did the right thing, using the final output file, and input was concat.
        concatCall = [i for i in ffcalls if i.output == "/output.ts"]
        self.assertEqual(len(concatCall), 1)
        concatCall = concatCall[0]
        self.assertEqual(concatCall.input, "concat:"+outputs[:-1])  

if __name__ == '__main__':
    unittest.main()