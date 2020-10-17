FROM archlinux:latest

VOLUME /config

RUN mkdir /app 

RUN pacman --noconfirm -Syu
RUN pacman --noconfirm -Sy opencv python-requests python-numpy python-flask hdf5 qt5-base
COPY main.py /app/

#RUN pip install -r /app/requirements.txt

ENV CONFIG_FILE /config/config.json

CMD ["/usr/bin/python", "/app/main.py"]
