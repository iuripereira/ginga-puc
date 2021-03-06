# cfg.mk -- Setup maintainer's makefile.
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

COPYRIGHT_YEAR= 2017
COPYRIGHT_HOLDER= PUC-Rio/Laboratorio TeleMidia

INDENT= clang-format
INDENT_OPTIONS= -i
INDENT_EXCLUDE= COPYING contrib/% src/ginga.h
INDENT_JOIN_EMPTY_LINES_EXCLUDE= $(INDENT_EXCLUDE)
SC_USELESS_IF_BEFORE_FREE_ALIASES= g_free

SYNTAX_CHECK_EXCLUDE= contrib/% examples/%
UPDATE_COPYRIGHT_EXCLUDE= $(SC_COPYRIGHT_EXCLUDE)
SC_COPYRIGHT_EXCLUDE=\
  $(REMOTE_FILES)\
  contrib/%\
  src/nclconv/DOMTreeErrorReporter.h\
  src/system/PracticalSocket.h\
  src/util/Base64.h\
  $(NULL)

SC_RULES+= sc-copyright
sc-copyright:
	$(V_at)$(build_aux)/syntax-check-copyright\
	  -b='/*' -e='*/' -t=cfg.mk\
	  $(call vc_list_exclude, $(VC_LIST_C), $(SC_COPYRIGHT_EXCLUDE))
	$(V_at)$(build_aux)/syntax-check-copyright\
	  -b='#' -t=cfg.mk\
	  $(call vc_list_exclude,\
	    $(VC_LIST_AC)\
	    $(VC_LIST_AM)\
	    $(VC_LIST_MK)\
	    $(VC_LIST_PL)\
	    $(VC_LIST_SH),\
	    $(SC_COPYRIGHT_EXCLUDE))

# Files copied from the NCLua project.
nclua:= https://github.com/gflima/nclua/raw/master
NCLUA_FILES+= build-aux/Makefile.am.common
NCLUA_FILES+= build-aux/Makefile.am.coverage
NCLUA_FILES+= build-aux/Makefile.am.gitlog
NCLUA_FILES+= build-aux/Makefile.am.valgrind
NCLUA_FILES+= build-aux/util.m4
NCLUA_FILES+= maint.mk
NCLUA_SCRIPTS+= bootstrap
NCLUA_SCRIPTS+= build-aux/syntax-check
NCLUA_SCRIPTS+= build-aux/syntax-check-copyright
REMOTE_FILES+= $(NCLUA_FILES)
REMOTE_SCRIPTS+= $(NCLUA_SCRIPTS)

.PHONY: fetch-remote-local-nclua
fetch-remote-local-nclua:
	$(V_at)for path in $(NCLUA_FILES) $(NCLUA_SCRIPTS); do\
	  if test "$$path" = "lib/luax-macros.h"; then\
	    dir=play;\
	  else\
	    dir=`dirname "$$path"`;\
	  fi;\
	  $(FETCH) -dir="$$dir" "$(nclua)/$$path" || exit 1;\
	done

fetch-remote-local: fetch-remote-local-nclua


# Generates TAGS file.
.PHONY: etags
etags:
	find . -type f \( -name '*.cpp' -or -name '*.h' \) -print\
	  | etags - --declarations


# Lists missing ginga header includes.
.PHONY: list-missing-ginga-h
list-missing-config-h:
	@for src in `find . -name '*.h'`; do\
	  fgrep -q '#include "ginga.h"' "$$src" || echo "$$src";\
	done


# Lists unused sources (.cpp).
.PHONY: list-unused-sources
list-unused-sources:
	@out="./$@.tmp";\
	cat `find . -name 'Makefile.am'` >"$$out";\
	for src in `find . -name '*.cpp'`; do\
	  egrep -q  "\<`basename $$src`\>" "$$out";\
	done;\
	rm -f "$$out";


# Lists unused headers (.h).
.PHONY: list-unused-headers
list-unused-headers:
	@out="./$@.tmp";\
	cat `find . \( -name '*.cpp' -or -name '*.h' \) ` >"$$out";\
	for h in `find . -name '*.h'`; do\
	  fgrep -q  "`basename $$h`" "$$out" || echo "$$h";\
	done;\
	rm -f "$$out";
