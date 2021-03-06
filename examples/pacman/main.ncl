<?xml version='1.0' encoding='utf-8'?>
<!-- Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>. -->

<ncl>
  <head>
    <regionBase>
      <region id='rgLua' top='0' left='0' width='100%' height='100%' zIndex='1' />
    </regionBase>
    <descriptorBase>
      <descriptor id='dLua' region='rgLua' focusIndex='0' />
    </descriptorBase>
  </head>
  <body>
    <port id='start' component='lua' />
    <media type='application/x-ginga-settings' id='settings'>
      <property name='service.currentKeyMaster' value='lua'/>
    </media>
    <media id='lua' src='game.lua' descriptor='dLua' />
  </body>
</ncl>
