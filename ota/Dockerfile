FROM archlinux:latest

RUN echo "[multilib]" >> /etc/pacman.conf
RUN echo "Include = /etc/pacman.d/mirrorlist" >> /etc/pacman.conf

RUN pacman --noconfirm -Sy archlinux-keyring
RUN pacman --noconfirm -Syu
RUN pacman --noconfirm -Syu python python-pip ffmpeg wine unzip

ENV LANG C.UTF-8

ADD src /postprocess
RUN cd /postprocess && pip3 install -r requirements.txt


ADD comskip.zip /
RUN mkdir -p /usr/share/comcut && cd /usr/share/comcut && unzip /comskip.zip && rm /comskip.zip

