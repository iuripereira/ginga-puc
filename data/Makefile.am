# Makefile.am -- Template for generating Makefile via Automake.
# Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia
#
# This file is part of Ginga (Ginga-NCL).
#
# Ginga is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Ginga is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Ginga.  If not, see <http://www.gnu.org/licenses/>.

include $(top_srcdir)/build-aux/Makefile.am.common

# button
button_DATA=\
  images/pauseButton.png\
  images/playButton.png\
  images/stopButton.png\
  $(NULL)

# contextmanager
contextmanager_DATA=\
  contextmanager/users.ini\
  contextmanager/contexts.ini\
  $(NULL)

# font
font_DATA=\
  fonts/decker.ttf\
  fonts/vera.ttf\
  $(NULL)

# formatter
formatterimg_DATA=\
  formatterimg/loading.png\
  formatterimg/b1.png\
  formatterimg/b2.png\
  formatterimg/b3.png\
  formatterimg/b4.png\
  formatterimg/b6.png\
  formatterimg/b5.png\
  formatterimg/b7.png\
  formatterimg/b8.png\
  $(NULL)

formattermd_DATA=\
  formattermd/active-device-reset.png\
  formattermd/active-device.png\
  $(NULL)

# ini
INTERNAL_DELIM= "/"
FOREIGN_DELIM= "\\\\"
INSTALL_PREFIX= "$(pkgdatadir)"
FILES_PREFIX= "$(pkgdatadir)"

pkgdata_DATA=\
  ini/mimetypes.ini\
  $(NULL)

# mb
mb_DATA=\
  mb/input.cfg\
  $(NULL)

# tuner
tuner_DATA=\
  tuner/tuner.ini\
  tuner/isdbt.ini\
  tuner/tuning.png\
  $(NULL)
