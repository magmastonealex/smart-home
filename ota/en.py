#! /usr/bin/python2

# en.py is responsible for doing com skipping and _en_queuing the job in ffproc.
# This script is _awful_. It can and should be combined with headendproc,
# and in general rethought, cleaned up, and tested.

import subprocess
import sys
import glob
import uuid
import os
import datetime
import json
import shutil

inpath = sys.argv[1]
seas = sys.argv[2]
ep = sys.argv[3]
title = sys.argv[4]

daterec = datetime.datetime.fromtimestamp(int(sys.argv[5]))

outpath = "/Media/tv/"+title+"/Season "+seas+"/"+title+".S"+seas+"E"+ep+".ts"
if str(seas)=="0" or str(ep)=="0":
	dt = daterec.strftime("%Y-%m-%d")
	outpath="/Media/tv/"+title+"/"+title+" - "+dt+".ts"

if os.path.exists(outpath):
	outpathsplit = outpath.split(".")
	outpathsplit[-2] = outpathsplit[-2]+".2"
	outpath=".".join(outpathsplit)

thisuuid=str(uuid.uuid4())
fil=os.path.basename(inpath)[:-3]

subprocess.call(["wine","/usr/share/comcut/comskip.exe","Z:\\Media\\myth\\"+fil+".ts"])
# You need to call comskip twice
if not os.path.exists("/Media/myth/"+fil+".edl"):
	subprocess.call(["wine","/usr/share/comcut/comskip.exe","Z:\\Media\\myth\\"+fil+".ts"])

# Clean up the mess left by comskip.
os.remove("/Media/myth/"+fil+".txt")
os.remove("/Media/myth/"+fil+".logo.txt")
os.remove("/Media/myth/"+fil+".log")
f=open("/Media/myth/"+fil+".edl")
parses=f.readlines()
f.close()
ending="0.0"
num=0

first=1

for ln in parses:
	ln=ln.rstrip()
	times=ln.split("\t")
	if len(times) < 3:
		continue
	if times[0] == '0.00':
		pass
	else:
		if ending != "":
			print times[0]
			print "Encoding: -ss: "+ending+" -t: "+str((float(times[0])-float(ending))+4)
			subprocess.call(["ffmpeg", "-i", "/Media/myth/"+fil+".ts","-acodec", "copy", "-vcodec", "copy", "-threads", "0",  "-avoid_negative_ts", "make_zero", "-fflags","+genpts", "-f", "mpegts", "-ss", ending, "-t",str(float(times[0])-float(ending)+3.0), "/Media/tmp/comskipping_"+thisuuid+"_"+str(num)+".ts"])
			num=num+1
	ending=times[1]

subprocess.call(["ffmpeg", "-i", "/Media/myth/"+fil+".ts","-acodec", "copy", "-vcodec", "copy", "-threads", "0", "-avoid_negative_ts", "make_zero", "-fflags","+genpts", "-f", "mpegts", "-ss", ending, "/Media/tmp/comskipping_"+thisuuid+"_"+str(num)+".ts"])
print "Finished cutting, current # segments:  " + str(num)

finalline=""
for x in range(num+1):
	finalline=finalline+"/Media/tmp/comskipping_"+thisuuid+"_"+str(x)+".ts|"

print finalline

subprocess.call(["mkdir","-p",os.path.dirname(outpath)])
subprocess.call(["ffmpeg", "-i", "concat:"+finalline[:-1], "-f", "mpegts", "-vcodec", "copy", "-acodec", "copy", "-avoid_negative_ts", "make_zero", "-fflags", "+genpts", out])
for x in range(num+1):
	try:
		os.remove("/Media/tmp/comskipping_"+thisuuid+"_"+str(x)+".ts")
	except:
		pass


istream=json.loads(subprocess.Popen(["/usr/bin/ffprobe","-v", "quiet", "-print_format", "json", "-show_streams","/Media/myth/"+fil+".ts"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT).communicate()[0])["streams"]
edir=float(istream[0]["duration"])*0.55

ostream=json.loads(subprocess.Popen(["/usr/bin/ffprobe","-v", "quiet", "-print_format", "json", "-show_streams",out], stdout=subprocess.PIPE, stderr=subprocess.STDOUT).communicate()[0])["streams"]
rdir=float(ostream[0]["duration"])

if rdir < edir:
	print "Bad comskip."
	os.remove(outpath)
	shutil.copyfile("/Media/myth/"+fil+".ts",outpath)
else:
	print "Good comskip. Leaving."
	filin=fil

subprocess.call(["ccextractor","-ve",outpath, "-o", outpath[:-3]+".en.srt"])
subprocess.call(["chmod", "a+wr", outpath[:-3]+".en.srt"])

os.chdir("/usr/share/ffproc")
subprocess.call(["/usr/bin/python2", "./ffproc.py", outpath])