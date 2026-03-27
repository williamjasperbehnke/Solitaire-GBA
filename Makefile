#---------------------------------------------------------------------------------------------------------------------
# TARGET is the name of the output.
# BUILD is the directory where object files & intermediate files will be placed.
# LIBBUTANO is the main directory of butano library.
#---------------------------------------------------------------------------------------------------------------------
TARGET          :=  $(notdir $(CURDIR))
BUILD           :=  build
LIBBUTANO       :=  third_party/butano/butano
PYTHON          :=  python
SOURCES         :=  src src/solitaire/app src/solitaire/core src/solitaire/game src/solitaire/input src/solitaire/render
INCLUDES        :=  include third_party/butano/common/include
DATA            :=
GRAPHICS        :=  graphics third_party/butano/common/graphics
AUDIO           :=  audio
AUDIOBACKEND    :=  maxmod
AUDIOTOOL       :=
DMGAUDIO        :=  dmg_audio
DMGAUDIOBACKEND :=  default
ROMTITLE        :=  SOLITAIRE
ROMCODE         :=  SLTR
USERFLAGS       :=
USERCXXFLAGS    :=
USERASFLAGS     :=
USERLDFLAGS     :=
USERLIBDIRS     :=
USERLIBS        :=
DEFAULTLIBS     :=
STACKTRACE      :=
USERBUILD       :=
EXTTOOL         :=

ifndef LIBBUTANOABS
	export LIBBUTANOABS := $(realpath $(LIBBUTANO))
endif

include $(LIBBUTANOABS)/butano.mak
