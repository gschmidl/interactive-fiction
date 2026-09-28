This is the part of Strawberry Perl (perl 5.32.1, MSWin32-x64-multi-thread) that
Explore uses: perl.exe and its DLLs in bin, and in lib the 40 library files
(and 7 XS DLLs) that the game was seen to load.  src\mkruntime.pl made it;
it is meant to run the port's explore.pl and nothing else.  Config.pm and
Config_heavy.pl name C:\strawberry, Strawberry's own default folder, where
an installed Strawberry Perl names the folder it is in.  The licences of
Perl and of the gcc runtime DLLs are in licenses.
