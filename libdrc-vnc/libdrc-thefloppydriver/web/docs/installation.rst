Installation
============

System requirements
-------------------

Building libdrc requires a few system tools and libraries to be installed:

* A C++11 compiler.
* A recent version of ffmpeg and its libraries (libavutil, libswscale).
* If you want to build the libdrc demos, SDL and GLEW are also required.

To install a recent version of ffmpeg on Ubuntu 13.10::

    sudo add-apt-repository ppa:samrog131/ppa
    sudo apt-get update
    sudo apt-get install libswscale-ffmpeg-dev libavutil-ffmpeg-dev

Patched dependencies
--------------------

Because the Wii U GamePad is not using completely standard H.264, a patched
version of x264 is required to be able to build libdrc. This patched version
can be found on `memahaxx/drc-x264 <https://bitbucket.org/memahaxx/drc-x264>`_.

::

    git clone https://bitbucket.org/memahaxx/drc-x264/
    cd drc-x264
    ./configure --prefix=$PATCHED_X264_PREFIX --enable-static
    make install

Building libdrc
---------------

The most recent version of libdrc can be found on the main development
repository: `memahaxx/libdrc <https://bitbucket.org/memahaxx/libdrc>`_.

::

    git clone https://bitbucket.org/memahaxx/libdrc
    cd libdrc
    PKG_CONFIG_PATH=$PATCHED_X264_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH ./configure
    make

You should then be able to run the libdrc demos from the `demos` directories.
