FROM devkitpro/devkitarm
WORKDIR /root
RUN git clone https://github.com/rhaleblian/dslibris
RUN wget https://download.savannah.nongnu.org/releases/freetype/freetype-old/freetype-2.4.8.tar.gz
RUN tar xvf freetype-2.4.8.tar.gz
WORKDIR /root/freetype-2.4.8
RUN . /opt/devkitpro/ndsvars.sh && ./configure --prefix=/opt/nds/freetype-2.4.8 --host=arm-none-eabi --disable-shared --enable-static --with-zlib --with-bzip2
RUN make
RUN make install
RUN tar cf /root/freetype.tar /opt/nds
