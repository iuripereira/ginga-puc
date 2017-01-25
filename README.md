# Ginga

The iTV middleware.
* http://www.ginga.org.br
* http://www.ncl.org.br
* http://www.telemidia.puc-rio.br

### Known build errors

The berkelium package from Telemidia's repository are only for amd64.

libzip2 in Ubuntu 14.04, 14.10, 15.04, 15.10 and 16.04: These Ubuntu
versions not support libzip2 (required by multidevice).  Install libzip2
from launchpad repository as follows:

    wget http://launchpadlibrarian.net/201329489/libzip2_0.11.2-1.2_amd64.deb
    sudo dpkg -i libzip2_0.11.2-1.2_amd64.deb

FFmpeg in Ubuntu 14.04, 14.10 and 15.04: These Ubuntu versions not support
ffmpeg libraries.  Install ffmpegext from Telemidia's repository and apply
the patch as follows:

    sudo apt-get install ffmpegext
    ./contrib/debian/scripts/fix_configure_ac_ffmpeg.sh
