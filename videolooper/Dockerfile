FROM alpine:latest

VOLUME /recording
VOLUME /config

ADD videolooper /bin/videolooper

RUN apk update && apk add ffmpeg

HEALTHCHECK --timeout=3s CMD (ps aux | grep videolooper) || exit 1

CMD ["/bin/videolooper", "/config/config.json"]
