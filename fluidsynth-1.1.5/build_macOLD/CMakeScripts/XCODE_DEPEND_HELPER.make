# DO NOT EDIT
# This makefile makes sure all linkable targets are
# up-to-date with anything they link to
default:
	echo "Do not invoke directly"

# For each target create a dummy rule so the target does not have to exist
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Debug/libfluidsynth.a:
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/MinSizeRel/libfluidsynth.a:
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/RelWithDebInfo/libfluidsynth.a:
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Release/libfluidsynth.a:


# Rules to remove targets that are older than anything to which they
# link.  This forces Xcode to relink the targets from scratch.  It
# does not seem to check these dependencies itself.
PostBuild.fluidsynth.Debug:
PostBuild.libfluidsynth.Debug: /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Debug/fluidsynth
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Debug/fluidsynth:\
	/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Debug/libfluidsynth.a
	/bin/rm -f /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Debug/fluidsynth


PostBuild.libfluidsynth.Debug:
PostBuild.fluidsynth.Release:
PostBuild.libfluidsynth.Release: /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Release/fluidsynth
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Release/fluidsynth:\
	/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Release/libfluidsynth.a
	/bin/rm -f /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/Release/fluidsynth


PostBuild.libfluidsynth.Release:
PostBuild.fluidsynth.MinSizeRel:
PostBuild.libfluidsynth.MinSizeRel: /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/MinSizeRel/fluidsynth
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/MinSizeRel/fluidsynth:\
	/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/MinSizeRel/libfluidsynth.a
	/bin/rm -f /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/MinSizeRel/fluidsynth


PostBuild.libfluidsynth.MinSizeRel:
PostBuild.fluidsynth.RelWithDebInfo:
PostBuild.libfluidsynth.RelWithDebInfo: /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/RelWithDebInfo/fluidsynth
/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/RelWithDebInfo/fluidsynth:\
	/Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/RelWithDebInfo/libfluidsynth.a
	/bin/rm -f /Users/antoine/Code/fluidXtra/src/fluidsynth-1.1.5/build_mac/src/RelWithDebInfo/fluidsynth


PostBuild.libfluidsynth.RelWithDebInfo:
