FROM ubuntu:22.04
RUN apt update -y
RUN wget https://apt.devkitpro.org/install-devkitpro-pacman
RUN chmod +x install-devkitpro-pacman 
RUN ./install-devkitpro-pacman 
RUN rm ./install-devkitpro-pacman
RUN dkp-pacman -S $(cat requirements.txt)
RUN apt install -y doxygen
