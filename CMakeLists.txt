
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

########### Project name ###########
PROJECT(Stellarium)
SET(STELLARIUM_MAJOR "0")
SET(STELLARIUM_MINOR "13")
SET(STELLARIUM_PATCH "0")
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)

SET(PACKAGE_VERSION "${VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${PACKAGE_VERSION}")

SET(MINIMAL_QT_VERSION "5.1.0")

# Use customized cmake macros
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

########### Main global variables ###########
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug GProf Valgrind Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()

# Add gprof build options if necessary.  Note gmon.out will be created in working directory when Stellarium is executed
IF(${CMAKE_BUILD_TYPE} MATCHES "GProf")
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs")
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs")
 SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
ENDIF()

# Add valgrind build options if necessary
IF(${CMAKE_BUILD_TYPE} MATCHES "Valgrind")
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(APPLE 1)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
  # The stars structs rely on gnu gcc packing of bit-fields.
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-ms-bitfields")
  SET(GCC_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
ELSE()
  SET(GCC_VERSION "0.0")
ENDIF()

IF(WIN32)
  IF(NOT MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fexceptions -fident -mthreads")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wsign-promo -fexceptions -fident -mthreads")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wsign-promo -fexceptions -fident -mthreads -mwindows")    
  ELSE()
    # GZ: GCC flags unknown: exception handling should be default in MSVC.
    # it seems -fident does not exist any longer. -fno-ident is documented in gcc.
    # -mthreads and -mwindows are MinGW specific.
    # SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fexceptions -fident -mthreads -mwindows")

    # Prevent MSVC from complaining about M_PI no matter where <cmath> is included first
    add_definitions(-D_USE_MATH_DEFINES)
  ENDIF()
ELSE()
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unused-result")
ENDIF()

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wno-unused-private-field -Wno-uninitialized -Wno-tautological-constant-out-of-range-compare")
 # The cosmetic fix for Clang 3.4
 IF(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 3.3)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable")
 ENDIF()
ENDIF()

SET(OPTIMIZE_INTEL_ATOM 0 CACHE BOOL "Activate optimizations for atom processor.")
IF(OPTIMIZE_INTEL_ATOM)
 IF (MSVC)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2 ")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2 ")
 ELSE()
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=generic -msse -msse2 -msse3 -mfpmath=sse")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -msse -msse2 -msse3 -mfpmath=sse")
 ENDIF()
ENDIF()

# FreeBSD-specific compiler flags
# resolve bug for FreeBSD/amd64 and NVIDIA proprietary drivers 
IF(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  # Use -pthread compilation option to properly link to threading library
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  IF(${GCC_VERSION} VERSION_GREATER 4.7)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
  ENDIF()
ENDIF()

# best way to get to APPLE?
IF(APPLE)
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common -Wall -Wextra")
 IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-private-field -Wno-unknown-warning-option")
 ENDIF()
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -Wall -Wextra -Wno-unused-parameter")

 # We support only 64 bits MacOSX
 SET(CMAKE_OSX_ARCHITECTURES "x86_64")
ENDIF()

########### Others ###########
# Activate translation
SET(ENABLE_NLS 1 CACHE BOOL "Define whether program translation should be supported.")
IF(ENABLE_NLS)
  ADD_DEFINITIONS(-DENABLE_NLS)
ENDIF()

SET(RELEASE_BUILD 1 CACHE BOOL "Flag this build as an official release.")

# Activate sound support
SET(ENABLE_SOUND 0 CACHE BOOL "Define whether sound support should be activated.")

SET(ENABLE_SCRIPTING 1 CACHE BOOL "Define whether scripting features should be activated.")
IF(ENABLE_SCRIPTING)
  # (De-)Activate the script edit console
  SET(ENABLE_SCRIPT_CONSOLE 1 CACHE BOOL "Define whether to build the script console feature.")
  IF(ENABLE_SCRIPT_CONSOLE)
	ADD_DEFINITIONS(-DENABLE_SCRIPT_CONSOLE)
  ENDIF()
  SET(ENABLE_STRATOSCRIPT_COMPAT 0 CACHE BOOL "Set to 1 or true if you want to build the Stratoscript compatibility mode")
ELSE()
  ADD_DEFINITIONS(-DDISABLE_SCRIPTING)
ENDIF()


SET(GUI_MODE Standard CACHE STRING "Choose the type of GUI to build, options are: Standard, None, External")

SET(GENERATE_PACKAGE_TARGET 1 CACHE BOOL "Set to 1 or true if you want to have make package target")

# On WIN32 we need to split the main binary into a small binary and a dll
# This is for allowing to link dynamic plug-ins afterward
# It does not seem to work with MSVC, so I disable it for the moment.
IF(WIN32 AND NOT MSVC)
  SET(GENERATE_STELMAINLIB 1)
ELSE()
  SET(GENERATE_STELMAINLIB 0)
ENDIF()

########### Plugin setup #############

SET(STELLARIUM_PLUGINS) # Global list of all the plugins.
MACRO(ADD_PLUGIN NAME DEFAULT)
  STRING(TOUPPER ${NAME} NAME_UP)
  SET(USE_PLUGIN_${NAME_UP} ${DEFAULT} CACHE BOOL "Define wheter the ${NAME} plugin should be created.")
  SET(STELLARIUM_PLUGINS ${STELLARIUM_PLUGINS} ${NAME})
ENDMACRO()

#### demo plugins ####
ADD_PLUGIN(HelloStelModule 0)
ADD_PLUGIN(SimpleDrawLine 0)
#### work plugins ####
ADD_PLUGIN(AngleMeasure 1)
ADD_PLUGIN(CompassMarks 1)
ADD_PLUGIN(Exoplanets 1)
ADD_PLUGIN(EquationOfTime 1)
ADD_PLUGIN(FOV 1)
ADD_PLUGIN(LogBook 0)
ADD_PLUGIN(MeteorShowers 1)
ADD_PLUGIN(NavStars 1)
ADD_PLUGIN(Novae 1)
ADD_PLUGIN(Observability 1)
ADD_PLUGIN(Oculars 1)
ADD_PLUGIN(Pulsars 1)
ADD_PLUGIN(Quasars 1)
ADD_PLUGIN(Satellites 1)
ADD_PLUGIN(SolarSystemEditor 1)
ADD_PLUGIN(Supernovae 1)
ADD_PLUGIN(SVMT 0)
ADD_PLUGIN(TextUserInterface 1)
ADD_PLUGIN(TelescopeControl 1)
ADD_PLUGIN(TimeZoneConfiguration 1)
ADD_PLUGIN(VirGO 0)

########## Static plugins need to define includes and libraries
########## for the compilation of Stellarium itself
IF(USE_PLUGIN_SVMT)
    SET(QT_USE_QTXML TRUE) # For SVMT we also need QtXml module
    SET(QT_USE_QTDECLARATIVE TRUE) # For SVMT we also need QML module, this line doesn't seem to work right now
    SET(QT_USE_QTWEBKIT TRUE) # For SVMT we also need QML module, this line doesn't seem to work right now
ENDIF()

IF(USE_PLUGIN_VIRGO)
    ## Library for loading FITS image files
    FIND_PATH(CFITSIO_INCLUDE_DIR fitsio.h
        /usr/include
        /usr/local/include
        DOC "Should point to the directory where the include files for libcfitsio are installed")
    FIND_LIBRARY(CFITSIO_LIBRARIES cfitsio
        PATHS
        /usr/lib/
        /usr/local/lib
        /opt/local/lib
        DOC "cfitsio link options")
    SET(QT_USE_QTXML TRUE) # For VirGO we also need QtXml module
ENDIF()

IF(USE_PLUGIN_LOGBOOK)
    SET(QT_USE_QTSQL TRUE) # For LogBook we also need SQL module
ENDIF()

# Custom target used to manage dependencies of stellarium -> Static plugins
# It is important that static plugins are compiled before stellarium main executable is linked
ADD_CUSTOM_TARGET(AllStaticPlugins ALL)

########### Find packages ###########
FIND_PACKAGE(Qt5Core REQUIRED)
GET_TARGET_PROPERTY(QMAKE_LOCATION Qt5::qmake LOCATION)
IF(${Qt5Core_VERSION_STRING} VERSION_LESS MINIMAL_QT_VERSION)
    MESSAGE(FATAL_ERROR "Found Qt5: ${QMAKE_LOCATION} (found unsuitable version \"${Qt5Core_VERSION_STRING}\", required is \"${MINIMAL_QT_VERSION}\")")
ELSE()
    MESSAGE(STATUS "Found Qt5: ${QMAKE_LOCATION} (found suitable version \"${Qt5Core_VERSION_STRING}\", required is \"${MINIMAL_QT_VERSION}\")")
ENDIF()
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5Declarative REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
IF(WIN32)
    FIND_PACKAGE(Qt5Sql REQUIRED)
    FIND_PACKAGE(Qt5XmlPatterns REQUIRED)
ENDIF()


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(ENABLE_SCRIPTING)
    SET(QT_USE_QTSCRIPT TRUE)
    FIND_PACKAGE(Qt5Script REQUIRED)
    INCLUDE_DIRECTORIES(${Qt5Script_INCLUDE_DIRS})
ENDIF()
IF(ENABLE_SOUND)
    ADD_DEFINITIONS(-DENABLE_SOUND)
    FIND_PACKAGE(Qt5Multimedia REQUIRED)
    INCLUDE_DIRECTORIES(${Qt5Multimedia_INCLUDE_DIRS})
ENDIF()

# I add this test because on Windows with angle we should not link
# with OpenGL.  Maybe there is a better way to do that.
IF("${Qt5Gui_OPENGL_LIBRARIES}" MATCHES "opengl")
    FIND_PACKAGE(OpenGL REQUIRED)
ENDIF()
FIND_PACKAGE(ZLIB)

FIND_PACKAGE(Qt5Test)
INCLUDE_DIRECTORIES(${Qt5Test_INCLUDE_DIRS})

########### Set some global variables ###########
IF(UNIX AND NOT WIN32)
    IF(APPLE)
        SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/Stellarium.app/Contents")
    ELSE()
        ADD_DEFINITIONS(-DINSTALL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/stellarium")
        ADD_DEFINITIONS(-DINSTALL_LOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")
        # Used in generating the documentation (doc/stellarium.pod.cmake):
        SET(INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/share/stellarium")
    ENDIF()
ELSE()
    ADD_DEFINITIONS(-DINSTALL_DATADIR=".")
    ADD_DEFINITIONS(-DINSTALL_LOCALEDIR="./locale")
ENDIF()

########### Get revision number for non-release builds ###########
IF(NOT RELEASE_BUILD)
    #Hack until CMake detection for Bazaar is finished
    IF(BZR_REVISION)
        SET(PACKAGE_VERSION "bzr${BZR_REVISION}")
        ADD_DEFINITIONS(-DBZR_REVISION="${BZR_REVISION}")
    ELSE()
        FIND_PACKAGE(Subversion)
        IF(Subversion_FOUND)
            Subversion_WC_INFO(${PROJECT_SOURCE_DIR} StelSvn)
            SET(SVN_REVISION ${StelSvn_WC_REVISION})
            ADD_DEFINITIONS(-DSVN_REVISION="${SVN_REVISION}")
        ENDIF()
    ENDIF()
ENDIF()

IF(ENABLE_STRATOSCRIPT_COMPAT)
    ADD_DEFINITIONS(-DENABLE_STRATOSCRIPT_COMPAT)
ENDIF()

### [Optional] Embed icon in the Windows executable ###
IF(WIN32)
    SET(ENABLE_WINDOWS_EXE_ICON 1 CACHE BOOL "Determine if it should try to embed the Stellarium icon in the Windows .exe file")
    IF(ENABLE_WINDOWS_EXE_ICON AND NOT RC_COMPILER_PATH)
        #The mingGW snapshot distributed with the Qt SDK has it under this name.
        SET(RC_COMPILER_FILENAME "windres.exe")
            FIND_FILE(RC_COMPILER_PATH ${RC_COMPILER_FILENAME})
        IF(RC_COMPILER_PATH)
            MESSAGE(STATUS "Found .rc compiler: ${RC_COMPILER_PATH}")
        ENDIF(RC_COMPILER_PATH)
    ENDIF(ENABLE_WINDOWS_EXE_ICON AND NOT RC_COMPILER_PATH)
ENDIF()

### Generate an Inno Setup project file ###
IF(WIN32)
    # Try to guess the MinGW /bin directory...
    GET_FILENAME_COMPONENT(MINGW_BIN_DIRECTORY ${CMAKE_CXX_COMPILER} PATH)
    IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
        SET(ISS_ARCHITECTURE_SPECIFIC ";In the 64-bit only version, this line switches the installer to 64-bit mode.")
        SET(ISS_PACKAGE_PLATFORM "win32")
        IF(MSVC)
          SET(REDIST "Source: \"${CMAKE_INSTALL_PREFIX}/bin/msvc*.dll\"; DestDir: \"{app}\";")
        ELSE()
          SET(REDIST "Source: \"${MINGW_BIN_DIRECTORY}/libstdc++*.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libgcc_s_dw2-1.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libwinpthread*.dll\"; DestDir: \"{app}\";")
        ENDIF()
    ELSEIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        SET(ISS_ARCHITECTURE_SPECIFIC ";Make the installer run only on win64:\nArchitecturesAllowed=x64\n;Switch to 64-bit install mode:\nArchitecturesInstallIn64BitMode=x64")
        SET(ISS_PACKAGE_PLATFORM "win64")
        IF(MSVC)
          SET(REDIST "Source: \"${CMAKE_INSTALL_PREFIX}/bin/msvc*.dll\"; DestDir: \"{app}\";")
        ELSE()
          SET(REDIST "Source: \"${MINGW_BIN_DIRECTORY}/libstdc++*.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libwinpthread*.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libgcc_s_sjlj*.dll\"; DestDir: \"{app}\";")
        ENDIF()
    ENDIF()
    SET(ISS_AUTOGENERATED_WARNING "Do not edit this file! It has been automatically generated by CMake. Your changes will be lost the next time CMake is run.")
    IF(MSVC)
        SET(ZLIB "; ZLIB already included...")
    ELSE()
        SET(ZLIB "Source: \"${ZLIB_INCLUDE_DIR}/../bin/zlib1.dll\"; DestDir: \"{app}\";")
    ENDIF()
    IF(GENERATE_STELMAINLIB)
        SET(STELMAINLIB "Source: \"${CMAKE_INSTALL_PREFIX}/lib/libstelMain.dll\"; DestDir: \"{app}\";")
    ELSE()
        SET(STELMAINLIB "; StelMainLib don't used")
    ENDIF()
    GET_TARGET_PROPERTY(QtConcurrent_location Qt5::Concurrent LOCATION)
    GET_TARGET_PROPERTY(QtCore_location Qt5::Core LOCATION)
    GET_TARGET_PROPERTY(QtGui_location Qt5::Gui LOCATION)
    GET_TARGET_PROPERTY(QtOpenGL_location Qt5::OpenGL LOCATION)
    GET_TARGET_PROPERTY(QtNetwork_location Qt5::Network LOCATION)
    GET_TARGET_PROPERTY(QtWidgets_location Qt5::Widgets LOCATION)
    GET_TARGET_PROPERTY(QtDeclarative_location Qt5::Declarative LOCATION)
    GET_TARGET_PROPERTY(QtSql_location Qt5::Sql LOCATION)
    GET_TARGET_PROPERTY(QtSvg_location Qt5::Svg LOCATION)
    GET_TARGET_PROPERTY(QtXmlPatterns_location Qt5::XmlPatterns LOCATION)
    IF(ENABLE_SCRIPTING)
        GET_TARGET_PROPERTY(QtScript_location Qt5::Script LOCATION)
        SET(ISS_QT_SCRIPT "Source: \"${QtScript_location}\"; DestDir: \"{app}\";")
    ELSE()
        SET(ISS_QT_SCRIPT "; QtScript don't used")
    ENDIF()
    IF(ENABLE_SOUND)
        GET_TARGET_PROPERTY(QtMultimedia_location Qt5::Multimedia LOCATION)
        SET(ISS_QT_MULTIMEDIA "Source: \"${QtMultimedia_location}\"; DestDir: \"{app}\";")
    ELSE()
        SET(ISS_QT_MULTIMEDIA "; QtMultimedia don't used")
    ENDIF()
    GET_FILENAME_COMPONENT(QT5_LIBS ${QtCore_location} PATH)
    IF(MSVC)
        SET(ISS_ANGLE_LIBS "; ANGLE support (libEGL.dll, libGLESv2.dll and d3dcompiler_*.dll)\nSource: \"${QT5_LIBS}/libEGL.dll\"; DestDir: \"{app}\";\nSource: \"${QT5_LIBS}/libGLESv2.dll\"; DestDir: \"{app}\";\nSource: \"${QT5_LIBS}/d3dcompiler_*.dll\"; DestDir: \"{app}\";")
    ELSE()
        SET(ISS_ANGLE_LIBS "; ANGLE support doesn't exists for MinGW")
    ENDIF()
    SET(ISS_ICU_LIBS "; ICU support\nSource: \"${QT5_LIBS}/icu*.dll\"; DestDir: \"{app}\";")
    # Deploy related stuff
    SET(ISS_WINDOWS_PLUGIN "Source: \"${QT5_LIBS}/../plugins/platforms/qwindows.dll\"; DestDir: \"{app}/platforms/\";")
    SET(ISS_QML_DIR "Source: \"${QT5_LIBS}/../imports/Qt/labs/shaders/qmldir\"; DestDir: \"{app}/Qt/labs/shaders/\";")
    SET(ISS_QML_PLUGINS "Source: \"${QT5_LIBS}/../imports/Qt/labs/shaders/plugins.qmltypes\"; DestDir: \"{app}/Qt/labs/shaders/\";")
    SET(ISS_QML_SHADERS "Source: \"${QT5_LIBS}/../imports/Qt/labs/shaders/qmlshadersplugin.dll\"; DestDir: \"{app}/Qt/labs/shaders/\";")
  
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/stellarium.iss.cmake ${CMAKE_SOURCE_DIR}/stellarium.iss @ONLY)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/stellarium-patch.iss.cmake ${CMAKE_SOURCE_DIR}/stellarium-patch.iss @ONLY)
ENDIF()

IF(APPLE)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/data/Info.plist.cmake ${CMAKE_SOURCE_DIR}/data/Info.plist @ONLY)
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    ADD_DEFINITIONS(-DQT_NO_DEBUG)
    ADD_DEFINITIONS(-DNDEBUG)
ENDIF()

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/data/default_config.ini.cmake ${CMAKE_SOURCE_DIR}/data/default_config.ini @ONLY)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/data/updates.json.cmake ${CMAKE_SOURCE_DIR}/data/updates.json @ONLY)

########### uninstall files ###############
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

########### Packaging info for CPack ###########

IF(GENERATE_PACKAGE_TARGET)
  INCLUDE(InstallRequiredSystemLibraries)
  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Stellarium is a free open source planetarium for your computer. It shows a realistic sky in 3D, just like what you see with the naked eye, binoculars or a telescope.")
  SET(CPACK_PACKAGE_VENDOR "Stellarium's team")
  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
  SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  SET(CPACK_PACKAGE_VERSION_MAJOR ${STELLARIUM_MAJOR})
  SET(CPACK_PACKAGE_VERSION_MINOR ${STELLARIUM_MINOR})
  SET(CPACK_PACKAGE_VERSION_PATCH ${STELLARIUM_PATCH})
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "stellarium")
  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "stellarium-${VERSION}")
  SET(CPACK_SOURCE_GENERATOR "TGZ")
  SET(CPACK_GENERATOR "TGZ")
  SET(CPACK_STRIP_FILES "bin/stellarium")
  SET(CPACK_PACKAGE_EXECUTABLES "stellarium" "Stellarium")
  SET(CPACK_SOURCE_IGNORE_FILES "/CVS/" "/.svn/" "/.git/" "/.bzr/" "builds/" "installers/" "Stellarium.tag$" "Stellarium.kdevelop.pcs$" "/CMakeLists.txt.user$" "\\\\.bzrignore$" "~$" "\\\\.swp$" "\\\\.#" "/#")
  SET(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
  SET(CPACK_RPM_PACKAGE_GROUP "Amusements/Graphics")
  SET(CPACK_RPM_PACKAGE_URL "http://stellarium.org/")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <stellarium-pubdevel@lists.sourceforge.net>")
  SET(CPACK_DEBIAN_PACKAGE_SECTION "science")
  SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION}+deb1")
  SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_RPM_PACKAGE_URL}")
  SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  INCLUDE(CPack)
ENDIF()

########### Macosx Bundling ###############
IF(APPLE)
	# ${Qt5Core_INCLUDE_DIRS} is a list; I'm expecting the first entry to always be QtCore.framework.
	ADD_CUSTOM_TARGET(
		mac_app
		python util/mac_app.py ${CMAKE_INSTALL_PREFIX} ${PROJECT_SOURCE_DIR} ${CMAKE_BUILD_TYPE} ${Qt5Core_INCLUDE_DIRS} 
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Stellarium.app
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		COMMENT "making the macosx bundle."
		VERBATIM
	)
ENDIF()


########### Generate doxygen doc ###############
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
ADD_CUSTOM_TARGET(apidoc doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generate the doxygen documentation into the doc directory.")

##################### Generate translation copying script ######################
IF(WIN32)
 STRING(REPLACE "/" "\\" PROJECT_SOURCE_DIR_WINPATH ${PROJECT_SOURCE_DIR})
 STRING(REPLACE "/" "\\" PROJECT_BINARY_DIR_WINPATH ${PROJECT_BINARY_DIR})
 CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/util/copy-translations.bat.cmake ${PROJECT_SOURCE_DIR}/util/copy-translations.bat @ONLY NEWLINE_STYLE WIN32)
ENDIF()

########### Top level include directories ###########
# This will be used for all compilations in sub-directories
INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/core
	${CMAKE_SOURCE_DIR}/src/core/modules
	${CMAKE_SOURCE_DIR}/src/core/planetsephems
	${CMAKE_SOURCE_DIR}/src/core/external
	${CMAKE_SOURCE_DIR}/src/core/external/kfilter
	${CMAKE_SOURCE_DIR}/src/core/external/glues_stel/source/
	${CMAKE_SOURCE_DIR}/src/core/external/glues_stel/source/libtess
	${CMAKE_SOURCE_DIR}/src/gui
	${CMAKE_SOURCE_DIR}/src/scripting
	${INTL_INCLUDE_DIR}
	${CMAKE_BINARY_DIR}/src
	${ZLIB_INCLUDE_DIR}
)

########### Subdirectories ###############
ADD_SUBDIRECTORY( data )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( textures )
ADD_SUBDIRECTORY( doc )
IF(ENABLE_NLS)
  ADD_SUBDIRECTORY( po )
ENDIF()
ADD_SUBDIRECTORY( landscapes )
ADD_SUBDIRECTORY( skycultures )
ADD_SUBDIRECTORY( nebulae )
IF(ENABLE_SCRIPTING)
  ADD_SUBDIRECTORY( scripts )
ENDIF()
ADD_SUBDIRECTORY( stars )
ADD_SUBDIRECTORY( plugins )
