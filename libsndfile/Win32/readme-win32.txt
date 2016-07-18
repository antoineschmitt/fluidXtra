Compiling the Libsnd library for WIN32 platforms, using the MSVC compiler:

Open the libsndfile.dsw workspace in MS developers studio
Select Build->Rebuild All from the main menu to compile the library,
including the examples and test programs.

How to use LibSnd in your own project?
Just insert the libsndfile.dsp file into your project (Project->
Insert project into Workspace menu option). Select the project
that needs the LibSndFile.lib, and do the following:

1) Select the project->settings option
3) Select the C/C++ tab, and select the preprocessor option
4) Add a path to libsnd\Win32 directory and to the libsnd\src directory

Select the OK button and your all set.

Some notes:
If you want to use a different compiler, make sure the alignment is set to 1


Albert Faber


