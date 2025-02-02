 FluidXtra readme
Antoine Schmitt May 2004 - Jan 2005

This Xtra gives access to all the features of fluidsynth inside Macromedia Director.

- FluidXtra links with the fluidsynth.lib static library

- FluidXtra depends on libsndfile, of which it includes some source files. libsndfile is a LGPL project for reading and writing sound files.
libsndfile project site : http://www.mega-nerd.com/libsndfile/

- FluidXtra depends of course on Director's Xtra Development Kit (XDK) : http://www.macromedia.com/support/xtras/
This version was built and tested against Director 8.5 and XDK8.5, but should work with earlier versions.

- FluidXtra was compiled on:
- MacOS9 using CodeWarrior Pro 5
- MacOSX 10.3 using CodeWarrior Pro 7
- Windows using VisualStudio.Net2003

// ----------------------------------------------------------------------------
// -- Compilation of fluidXtra with fluidsynth1.1.5 (dependent of glib) on Windows
// ----------------------------------------------------------------------------

1) to build the glib static library
- download glib_src, zlib_dev and proxy-libintl-dev from the gtk web site
- create a vs10 folder alongside the glib-2.28.8 folder
- install (decompress) zlib_dev and proxy-libintl-dev as documented, in vs10
"The required dependencies are zlib and proxy-libintl. Fetch the latest proxy-libintl-dev and zlib-dev zipfiles from
http://ftp.gnome.org/pub/GNOME/binaries/win32/dependencies/ for 32-bit
builds"
- the props are wrong wrt/ the location of the v10 folder.
	- open the glib-2.28.8\build\win32\vs10\glib.props(453) file and change GlibEtcInstallRoot and CopyDir  to point one level below
		<CopyDir>..\..\..\..\vs10\$(Platform)</CopyDir>
		<GlibEtcInstallRoot>..\..\..\..\vs10\$(Platform)</GlibEtcInstallRoot>
- open the glib/build/.../vs10 solution
	- modify the output type to .lib
	- remove the DLL_EXPORT macro
	- set the RUNTIME library to "DEBUG MULTITHREAD"
	- check if you build Debug or Release : you should build the fluidXtra with the same setting
- build

- same for gthread


2) to build fluidsynth1.1.5
- download fluidsynth1.1.5
- use CMake to create the sln
- open the solution generated by cmake
- in win_build/config.h : set #define PORTAUDIO_SUPPORT 1
- in the properties
	- set the RUNTIME library to "DEBUG MULTITHREAD"
	- verify the glib include and lib paths
    ${CMAKE_SOURCE_DIR}/../glib_dev\vs10\Win32\include
    ${CMAKE_SOURCE_DIR}/../glib_dev\vs10\Win32\include\glib-2.0
    ${CMAKE_SOURCE_DIR}/../glib_dev\vs10\Win32\lib\glib-2.0\include
	- if needed, replace the macro FLUIDSYNTH_DLL_EXPORTS by the macro FLUIDSYNTH_NOT_A_DLL
	- set "ensemble outils de plateforme" = "VS2012 Windows XP"- modify source:
	- in dsound.c : calls to fluid_win32_create_window and fluid_win32_destroy_window
	- in fluid_dll.c : fluid_win32_create_window 
- pour portaudio/asio
	- rajouter les fichiers
		asio.cpp asiodrivers.cpp asiolist.cpp
		pa_allocation.c pa_converters.c pa_dither.c pa_cpuload.c
		pa_front.c pa_process.c pa_ringbuffer.c pa_stream.c pa_trace.c
		pa_win_coinitialize.c pa_win_hostapis.c
		pa_win_util.c pa_win_waveformat.c pa_x86_plain_converters.c
 		pa_asio.cpp
	- rajouter drivers/portaudio.c
	- property add include files
		portaudio/include
		portaudio/src/common
		portauid/src/os/win
		ASIOSDK/common
		ASIOSDK/host
		ASIOSDK/host/pc
	- property preprocessor
		PA_USE_ASIO=1
	- pour les fichiers cpp : properties/C++/avanc�/compile as CPP
- build

3) building fluidXtra : normal compile in DEBUG mode
	- set the RUNTIME library to "DEBUG MULTITHREAD"
	- set "ensemble outils de plateforme" = "VS2012 Windows XP"

// ----------------------------------------------------------------------------
// -- Compilation of fluidXtra with fluidsynth1.1.5 (dependent of glib) on Mac
// ----------------------------------------------------------------------------

- use homebrew  to install glib
	download homebrew
	edit the glib formula to disable avx
	brew edit glib
		 add this line to the 'install' entry
			ENV.append 'CFLAGS', '-mno-avx'
	brew install --universal glib

- get the /fluidsynth-1.1.5 source
- use CMake to create the xproj in fluidsynth-1.1.5/build_mac
	- only select coreaudio, unselect all the rest
	- select "no dynamic library" (at the top)
- open the xproj
	- set the GCC4.2 compiler (for __sync_synchronize, etc..)
	- choose the library target libfluidsynth, and BUILD

- open the fluidXtra xproj
	- check that we point on the right fluidsynth includes (fluidsynth-1.1.5/include and fluidsynth-1.1.5/build_mac/include) and lib (fluidsynth-1.1.5/build_mac/src/Debug)
	- compile the Xtra
- post build for the Xtra :
	- this copies the shlibs into the package, and links on them
	- do it in Terminal:

cd ..../products
./postbuildxtramac.command

which does this :
mkdir -p fluidXtra.xtra/Contents/Frameworks

cp /usr/local/Cellar/glib/2.30.3/lib/libgthread-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/
chmod +w fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib
install_name_tool -id @loader_path/../Frameworks/libgthread-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib
install_name_tool -change /usr/local/Cellar/glib/2.30.3/lib/libgthread-2.0.0.dylib @loader_path/../Frameworks/libgthread-2.0.0.dylib fluidXtra.xtra/Contents/MacOS/fluidXtra

cp /usr/local/Cellar/glib/2.30.3/lib/libglib-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/
chmod +w fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
install_name_tool -id @loader_path/../Frameworks/libglib-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
install_name_tool -change /usr/local/Cellar/glib/2.36.3/lib/libglib-2.0.0.dylib @loader_path/../Frameworks/libglib-2.0.0.dylib fluidXtra.xtra/Contents/MacOS/fluidXtra
install_name_tool -change /usr/local/Cellar/glib/2.36.3/lib/libglib-2.0.0.dylib @loader_path/../Frameworks/libglib-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib

cp /usr/lib/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/
chmod +w fluidXtra.xtra/Contents/Frameworks/libiconv.2.dylib
install_name_tool -id @loader_path/../Frameworks/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/libiconv.2.dylib
install_name_tool -change /usr/lib/libiconv.2.dylib @loader_path/../Frameworks/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
install_name_tool -change /usr/lib/libiconv.2.dylib @loader_path/../Frameworks/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib

cp /usr/local/opt/gettext/lib/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/
chmod +w fluidXtra.xtra/Contents/Frameworks/libintl.8.dylib
install_name_tool -id @loader_path/../Frameworks/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/libintl.8.dylib
install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib @loader_path/../Frameworks/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib @loader_path/../Frameworks/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib

// --- note : finding dependencies on Mac
otool -L fluidXtra.xtra/Contents/MacOS/fluidXtra 

