# Copyright 2026, Stephen Fryatt
#
# This file is part of Unify:
#
#   http://www.stevefryatt.org.uk/risc-os
#
# Licensed under the EUPL, Version 1.2 only (the "Licence");
# You may not use this work except in compliance with the
# Licence.
#
# You may obtain a copy of the Licence at:
#
#   http://joinup.ec.europa.eu/software/page/eupl
#
# Unless required by applicable law or agreed to in
# writing, software distributed under the Licence is
# distributed on an "AS IS" basis, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the Licence for the specific language governing
# permissions and limitations under the Licence.

# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

ARCHIVE := unify

APP := !Unify

#PACKAGE := Unify
#PACKAGELOC := Desktop

OBJS =  date_time.o	\
	file_instance.o	\
	file_set.o	\
	flexutils.o	\
	iconbar.o	\
	main.o		\
	suite.o		\
	textdump.o	\
	window.o

include $(SFTOOLS_MAKE)/CApp
