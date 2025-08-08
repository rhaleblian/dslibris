# docker buildx build . -t rhaleblian/nds-freetype:latest
FROM devkitpro/devkitarm
# ENV CFLAGS="-march=armv5te -mtune=arm946e-s -O2 -ffunction-sections -fdata-sections"
# ENV CXXFLAGS="${CFLAGS}"
# ENV CPPFLAGS="-D__NDS__ -DARM9 -I${PORTLIBS_PREFIX}/include -I${DEVKITPRO}/libnds/include"
# ENV LDFLAGS="-L${PORTLIBS_PREFIX}/lib -L${DEVKITPRO}/libnds/lib"
# ENV LIBS="-lnds9"
WORKDIR /root
RUN dkp-pacman -S nds-zlib nds-bzip2 --noconfirm
RUN wget https://download.savannah.nongnu.org/releases/freetype/freetype-old/freetype-2.6.5.tar.gz
RUN tar xvf freetype-2.6.5.tar.gz
WORKDIR /root/freetype-2.6.5
RUN . /opt/devkitpro/devkitarm.sh && ./configure --prefix=/opt/nds/freetype --host=arm-none-eabi --disable-shared --enable-static
RUN . /opt/devkitpro/devkitarm.sh && make
RUN . /opt/devkitpro/devkitarm.sh && make install
RUN tar cf /root/freetype.tar /opt/nds

# RUN git clone https://github.com/rhaleblian/dslibris
