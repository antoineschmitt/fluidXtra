#! /bin/bash

# change to current dir
if [ ! -f "postbuildxtramac.command" ] ; then
	newdir=`echo $0 | sed 's/postbuildxtramac.command//'`
	cd $newdir
fi

mkdir -p fluidXtra.xtra/Contents/Frameworks

lipo /usr/local/Cellar/glib/2.36.3/lib/libgthread-2.0.0.dylib -extract i386 -output fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib 
#cp /usr/local/Cellar/glib/2.36.3/lib/libgthread-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/
chmod +w fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib
install_name_tool -id @loader_path/../Frameworks/libgthread-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib
install_name_tool -change /usr/local/lib/libgthread-2.0.0.dylib @loader_path/../Frameworks/libgthread-2.0.0.dylib fluidXtra.xtra/Contents/MacOS/fluidXtra

lipo /usr/local/Cellar/glib/2.36.3/lib/libglib-2.0.0.dylib -extract i386 -output fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib 
#cp /usr/local/Cellar/glib/2.36.3/lib/libglib-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/
chmod +w fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
install_name_tool -id @loader_path/../Frameworks/libglib-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
install_name_tool -change /usr/local/lib/libglib-2.0.0.dylib @loader_path/../Frameworks/libglib-2.0.0.dylib fluidXtra.xtra/Contents/MacOS/fluidXtra
install_name_tool -change /usr/local/Cellar/glib/2.36.3/lib/libglib-2.0.0.dylib @loader_path/../Frameworks/libglib-2.0.0.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib

#cp /usr/lib/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/
#chmod +w fluidXtra.xtra/Contents/Frameworks/libiconv.2.dylib
#install_name_tool -id @loader_path/../Frameworks/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/libiconv.2.dylib
#install_name_tool -change /usr/lib/libiconv.2.dylib @loader_path/../Frameworks/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
#install_name_tool -change /usr/lib/libiconv.2.dylib @loader_path/../Frameworks/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib

lipo /usr/local/opt/gettext/lib/libintl.8.dylib -extract i386 -output fluidXtra.xtra/Contents/Frameworks/libintl.8.dylib 
#cp /usr/local/opt/gettext/lib/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/
chmod +w fluidXtra.xtra/Contents/Frameworks/libintl.8.dylib
install_name_tool -id @loader_path/../Frameworks/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/libintl.8.dylib
install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib @loader_path/../Frameworks/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/libglib-2.0.0.dylib
install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib @loader_path/../Frameworks/libintl.8.dylib fluidXtra.xtra/Contents/Frameworks/libgthread-2.0.0.dylib
#install_name_tool -change /usr/lib/libiconv.2.dylib @loader_path/../Frameworks/libiconv.2.dylib fluidXtra.xtra/Contents/Frameworks/libintl.8.dylib

echo postbuildxtramac.command : DONE