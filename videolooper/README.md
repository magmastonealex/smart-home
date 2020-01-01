videolooper
===

Videolooper was born out of frustration with existing NVR platforms. I had very limited requirements - I want to record the 10 seconds before, and 20 seconds after a particular event happens. The camera has a built-in motion detector, and the door it's watching has a [door sensor](../frontdoor).

Shinobi is too unreliable, often just giving up on recording, and ZoneMinder is way too heavy for what I'm doing. I had a Windows VM to run Milestone, which worked great, but is incredibly overkill for a single camera.

So, this uses ffmpeg to watch an RTSP (or any!) camera stream, constantly recording to an in-memory circular buffer (in mpeg-ts format for maximum hackiness). Upon receiving an event over MQTT, it dumps the contents of the buffer to disk, and continues appending the incoming bytes for the configured time interval. 

The resulting files are slightly weird (read: not-quite-standard, broken, etc.). They don't tend to start on keyframes, and sometimes the timeline is a bit messed up. You may want to postprocess with ffmpeg or play with mpv. It's good enough for my purposes.
