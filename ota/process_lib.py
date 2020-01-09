import pysubs2
import subprocess
import sys
import os
import json
import uuid
from loguru import logger
import glob

ffmpeg_common_args=["-hide_banner", "-loglevel", "quiet", "-stats"]

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
    
    dumpsubs = subprocess.run(["ffmpeg", *ffmpeg_common_args, "-f", "lavfi", "-i", "movie="+inname+"[out+subcc]", "-map", "0:1", "/tmp/" +subid +".ssa"])

    if dumpsubs.returncode != 0:
        logger.error("Ffmpeg failed!")
        return False

    logger.info("Loading and shifting...")
    try:
        subs = pysubs2.load("/tmp/" +subid +".ssa", encoding="utf-8")
        subs.shift(s=start_offset)
        subs.styles["Default"].fontname = "Arial"
        subs.styles["Default"].borderstyle = 1
        subs.save(outfile)
    except:
        logger.exception("failed to shift and reset subtitles")
        return False
    _try_remove("/tmp/" + subid + ".ssa")
    return True

# Attempts to os.remove filename, logging on failure.
def _try_remove(filename):
    try:
        os.remove(filename)
    except:
        logger.exception(filename + " file not found")

def _cleanup_comskipping(thisuuid, tempdir):
    files = glob.glob(tempdir+"/comskipping_" + thisuuid + "*")
    for file in files:
        os.remove(file)

def get_edl(filename, comskip_path):
    logger.info("Getting commercial skip points for %s. Comcut: %s" % (filename, comskip_path))
    basename = os.path.splitext(filename)[0]

    res = subprocess.run(["wine", comskip_path, "Z:" + filename])
    # Sometimes you need to run it twice - it may only generate the logo and then segfault.
    # But, sometimes it works.
    if not os.path.exists(basename + ".edl"):
        res = subprocess.run(["wine", comskip_path, "Z:" + filename])
    
    if res.returncode > 1 or not os.path.exists(basename + ".edl"):
        logger.error("Failed to detect commercials. Returning empty cutlist.")
        return []

    if res.returncode == 0:
        logger.info("No commercials found. Returning empty cutlist")
        return []

    _try_remove(basename + ".txt")
    _try_remove(basename + ".logo.txt")
    _try_remove(basename + ".log")
    
    f=open(basename + ".edl")
    parses=f.readlines()
    f.close()

    _try_remove(basename + ".edl")

    return parses

# Get the length, in float seconds of a given media file.   
def get_file_length(filename):
    probeproc = subprocess.Popen(["/usr/bin/ffprobe","-v", "quiet", "-print_format", "json", "-show_streams", filename], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout=probeproc.communicate()[0]
    if probeproc.returncode != 0:
        logger.warn("ffprobe returned %d" % (probeproc.returncodesss))
        return 0
    istream=json.loads(stdout)["streams"]
    return float(istream[0]["duration"])

# Regenerate PTS. It's often very high (since TV channels run for a long time!), and that causes issues for some players.
def postprocess_broadcast_ts(filename, outpath):
    subp = subprocess.run(["ffmpeg", *ffmpeg_common_args, "-i", filename, "-f", "mpegts", "-vcodec", "copy", "-acodec", "copy", "-muxdelay", "0", "-avoid_negative_ts", "make_zero", "-fflags", "+genpts", "-y", outpath])
    if subp.returncode != 0:
        return False
    return True

def cut_on_edl(filename, edl, outpath, tempdir):
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
                ffm=subprocess.run(["ffmpeg", *ffmpeg_common_args, "-i", filename,"-acodec", "copy", "-vcodec", "copy", "-muxdelay", "0", "-threads", "0",  "-avoid_negative_ts", "make_zero", "-fflags","+genpts", "-f", "mpegts", "-ss", ending, "-t",str(float(times[0])-float(ending)+3.0), tempdir + "/comskipping_"+thisuuid+"_"+str(num)+".ts"])
                if ffm.returncode != 0:
                    logger.warn("ffmpeg invoke failed")
                    _cleanup_comskipping(thisuuid, tempdir)
                    return False
                num=num+1
        ending=times[1]

    logger.info("Making final copy")
    ffm=subprocess.run(["ffmpeg", *ffmpeg_common_args, "-i", filename,"-acodec", "copy", "-vcodec", "copy", "-muxdelay", "0", "-threads", "0", "-avoid_negative_ts", "make_zero", "-fflags","+genpts", "-f", "mpegts", "-ss", ending, tempdir + "/comskipping_"+thisuuid+"_"+str(num)+".ts"])
    if ffm.returncode != 0:
        logger.warning("ffmpeg invoke failed")
        _cleanup_comskipping(thisuuid, tempdir)
        return False

    finalline=""
    for x in range(num+1):
        finalline=finalline+tempdir+"/comskipping_"+thisuuid+"_"+str(x)+".ts|"

    subprocess.run(["mkdir","-p",os.path.dirname(outpath)])
    concat = subprocess.run(["ffmpeg", *ffmpeg_common_args, "-i", "concat:"+finalline[:-1], "-f", "mpegts", "-vcodec", "copy", "-acodec", "copy", "-muxdelay", "0", "-avoid_negative_ts", "make_zero", "-fflags", "+genpts", "-y", outpath])
    if concat.returncode != 0:
        logger.warning("ffmpeg invoke failed")
        _cleanup_comskipping(thisuuid, tempdir)
        return False
 
    _cleanup_comskipping(thisuuid, tempdir)
    return True
