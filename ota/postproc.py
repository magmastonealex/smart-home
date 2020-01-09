from loguru import logger
import os
import sys

from process_lib import get_edl, cut_on_edl, get_file_length, process_subtitles, postprocess_broadcast_ts

PATH_TO_COMSKIP = "/usr/share/comcut/comskip.exe"
TEMPDIR = "/Media/tmp"

if "PATH_TO_COMSKIP" in os.environ:
    PATH_TO_COMSKIP = os.environ["PATH_TO_COMSKIP"]

if "TEMPDIR" in os.environ:
    TEMPDIR = os.environ["TEMPDIR"]


def main():
    if len(sys.argv) < 4:
        print("Usage: " + sys.argv[0] + " <input_filename> <comskip (true/false)> <output_filename>")
        print("Produces a synced SSA subtitle file for a given mpegts input.")
        sys.exit(2)

    inpath = sys.argv[1]
    comskip = sys.argv[2]
    outfile = sys.argv[3]

    logger.info("Starting processing %s, dest %s skipping %s" % (inpath, outfile, comskip))
    
    edl = ['735.28\t988.04\t0\n', '1430.91\t1706.92\t0\n', '2186.15\t2447.06\t0\n', '2793.99\t3079.76\t0\n', '3551.83\t3603.72\t0\n']
    if comskip != "false":
        edl = get_edl(inpath, PATH_TO_COMSKIP)
    print(edl)
    if len(edl) != 0:
        try:
            if cut_on_edl(inpath, edl, outfile, TEMPDIR) == False:
                raise RuntimeError("comskip failed")
            fllen_pre = get_file_length(inpath)
            fllen_post = get_file_length(outfile)
            logger.info("Original Length %f, final length %f" %(fllen_pre, fllen_post))
            if fllen_post < 0.55*fllen_pre or fllen_pre == 0.0 or fllen_post == 0.0:
                raise RuntimeError("More than half of file trimmed. Falling back to no comskip.")
        except:
            logger.exception("Failed to cut and process file. Falling back to straight postprocessing.")
            if postprocess_broadcast_ts(inpath, outfile) == False:
                logger.error("failed to postprocess into final location. This is a fatal error.")
    else:
        logger.info("no commercials found or comskipping not requested. Doing final re-processing...")
        # Run ffmpeg to re-timestamp the video
        if postprocess_broadcast_ts(inpath, outfile) == False:
            logger.warning("failed to postprocess into final location. This is a fatal error.")

    if process_subtitles(outfile, os.path.splitext(outfile)[0] + ".ssa") == False:
        logger.warning("Failed to extract subtitles from output file")
    
if __name__ == "__main__":
    main()
