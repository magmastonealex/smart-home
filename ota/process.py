import pysubs2
import subprocess
import sys
import os
import json
import uuid
from loguru import logger
import glob

PATH_TO_COMSKIP = "/usr/share/comcut/comskip.exe"
TEMPDIR = "/Media/tmp"

if "PATH_TO_COMSKIP" in os.environ:
    PATH_TO_COMSKIP = os.environ["PATH_TO_COMSKIP"]

if "TEMPDIR" in os.environ:
    TEMPDIR = os.environ["TEMPDIR"]

if len(sys.argv) < 4:
    print("Usage: " + sys.argv[0] + " <input_filename> <season> <episode>")
    print("Produces a synced SSA subtitle file for a given mpegts input.")
    sys.exit(2)

inpath = sys.argv[1]
season = sys.argv[2]
episode = sys.argv[3]

logger.info("Starting processing %s, S%sE%s" % (inpath, season, episode))

# Process an mpeg-ts file to remove subtitles in time-synced SSA format.
# inname should be a string path to the input file,
# and outfile should be a destination file - ending in .ssa
def process_subtitles(inname, outfile):
    logger.info("Getting start PTS offset...")
    proc = subprocess.Popen(["ffprobe", "-v", "quiet", "-print_format", "json", "-show_format", inname], stdout=subprocess.PIPE)
    out, err = proc.communicate()
    if proc.returncode != 0:
        logger.error("ffprobe failed!")
        return False

    start_offset = 0
    try:
        ffprobe_format = json.loads(out)
        start_offset = float(ffprobe_format["format"]["start_time"])
        logger.debug("Got " + str(start_offset))
    except:
        # This should at least return zero.
        logger.exception("Failed to load ffprobe output")
        return False

    subid = str(uuid.uuid4())
    logger.info("Transcoding to extract subtitles..")
    
    dumpsubs = subprocess.Popen(["ffmpeg", "-f", "lavfi", "-i", "movie="+inname+"[out+subcc]", "-map", "0:1", "/tmp/ " +subid +".ssa"])
    # Wait for transcode to finish..
    dumpsubs.communicate()

    if dumpsubs.returncode != 0:
        logger.error("Ffmpeg failed!")
        return False

    logger.info("Loading and shifting...")
    try:
        subs = pysubs2.load("/tmp/ " +subid +".ssa", encoding="utf-8")
        subs.shift(s=start_offset)
        subs.styles["Default"].fontname = "Arial"
        subs.styles["Default"].borderstyle = 1
        subs.save(outfile)
    except:
        logger.exception("failed to shift and reset subtitles")
        return False
    return True

# Attempts to os.remove filename, logging on failure.
def try_remove(filename):
    try:
        os.remove(filename)
    except:
        logger.exception(filename + " file not found")

def cleanup_comskipping(thisuuid):
    files = glob.glob(TEMPDIR+"/comskipping_" + thisuuid + "*")
    for file in files:
        os.remove(file)

def get_edl(filename):
    logger.info("Getting commercial skip points for %s. Comcut: %s" % (filename, PATH_TO_COMSKIP))
    basename = os.path.splitext(filename)[0]

    res = subprocess.run(["wine", PATH_TO_COMSKIP, "Z:" + filename])
    # Sometimes you need to run it twice - it may only generate the logo and then segfault.
    # But, sometimes it works.
    if not os.path.exists(basename + ".edl"):
        res = subprocess.run(["wine", PATH_TO_COMSKIP, "Z:" + filename])
    
    if res.returncode > 1 or not os.path.exists(basename + ".edl"):
        logger.error("Failed to detect commercials. Returning empty cutlist.")
        return []

    if res.returncode == 0:
        logger.info("No commercials found. Returning empty cutlist")
        return []

    try_remove(basename + ".txt")
    try_remove(basename + ".logo.txt")
    try_remove(basename + ".log")
    
    f=open(basename + ".edl")
    parses=f.readlines()
    f.close()

    try_remove(basename + ".edl")

    return parses

def get_file_length(filename):
    probeproc = subprocess.Popen(["/usr/bin/ffprobe","-v", "quiet", "-print_format", "json", "-show_streams", filename], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout=probeproc.communicate()[0]
    if probeproc.returncode != 0:
        logger.warn("ffprobe returned %d" % (probeproc.returncodesss))
        return 0
    istream=json.loads(stdout)["streams"]
    edir=float(istream[0]["duration"])*0.55

def cut_on_edl(filename, edl, outpath):
    logger.info("using EDL to cut %s" % (filename))
    ending="0.0"
    num=0

    thisuuid = str(uuid.uuid4())

    # Each line in the EDL contains a range we need to chop out.
    # This is a terrible algorithm that should be re-written to
    # get a bunch of files out that we need to keep.
    # Then, we use the concat filter to bring them all together.
    for ln in edl:
        ln=ln.rstrip()
        times=ln.split("\t")
        if len(times) < 3:
            continue
        if times[0] == '0.00':
            pass
        else:
            if ending != "":
                logger.info("copying: -ss: "+ending+" -t: "+str((float(times[0])-float(ending))+4))
                ffm=subprocess.run(["ffmpeg", "-i", filename,"-acodec", "copy", "-vcodec", "copy", "-threads", "0",  "-avoid_negative_ts", "make_zero", "-fflags","+genpts", "-f", "mpegts", "-ss", ending, "-t",str(float(times[0])-float(ending)+3.0), TEMPDIR + "/comskipping_"+thisuuid+"_"+str(num)+".ts"])
                if ffm.returncode != 0:
                    logger.warn("ffmpeg invoke failed")
                    cleanup_comskipping(thisuuid)
                    return False
                num=num+1
        ending=times[1]

    logger.info("Making final copy")
    ffm=subprocess.run(["ffmpeg", "-i", filename,"-acodec", "copy", "-vcodec", "copy", "-threads", "0", "-avoid_negative_ts", "make_zero", "-fflags","+genpts", "-f", "mpegts", "-ss", ending, TEMPDIR + "/comskipping_"+thisuuid+"_"+str(num)+".ts"])
    if ffm.returncode != 0:
        logger.warning("ffmpeg invoke failed")
        cleanup_comskipping(thisuuid)
        return False

    finalline=""
    for x in range(num+1):
        finalline=finalline+TEMPDIR+"/comskipping_"+thisuuid+"_"+str(x)+".ts|"

    subprocess.run(["mkdir","-p",os.path.dirname(outpath)])
    concat = subprocess.run(["ffmpeg", "-i", "concat:"+finalline[:-1], "-f", "mpegts", "-vcodec", "copy", "-acodec", "copy", "-avoid_negative_ts", "make_zero", "-fflags", "+genpts", "-y", outpath])
    if concat.returncode != 0:
        logger.warning("ffmpeg invoke failed")
        cleanup_comskipping(thisuuid)
        return False
 
    cleanup_comskipping(thisuuid)
    return True
