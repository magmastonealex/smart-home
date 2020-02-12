FROM python:3.8-alpine

VOLUME /config

RUN mkdir /app && apk --no-cache add tzdata && cp /usr/share/zoneinfo/America/Toronto /etc/localtime

COPY main.py requirements.txt /app/

RUN pip install -r /app/requirements.txt

ENV CONFIG_FILE /config/config.json

CMD ["/usr/local/bin/python", "/app/main.py"]
