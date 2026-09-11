Hello, Adventurer. Welcome to TOP-10 in a Box Version 1.1!

The purpose of this project is to make it as painless as possible for amateur and professional digital historians to have their own working and healthy virtual PDP-10 running the TOPS-10 operating system. While it's not quite one-click, it's as close as I can make it. This system includes working versions of BASIC and FORTRAN as well as the source and binary executables to two versions of the landmark game Adventure: Will Crowther's original from early 1976, as unearthed by Dennis Jerz a few years ago (http://digitalhumanities.org/dhq/vol/001/2/000009/000009.html), and the version completed and expanded by Don Woods in early 1977, which paralyzed IT departments all over the world for much of the remainder of that year and gave birth to a whole new genre of games. In addition, this starter system should make an ideal host for whatever other vintage PDP-10 software you care to unearth and install. I hope you find it useful!

INSTALLING TOPS-10 IN A BOX
===========================

1. Place all of the contents of the "TOPS-10 in a Box" zip file into a single folder somewhere on your computer. (You've quite possibly already done this.)

2. You'll need to acquire the SIMH project's PDP-10 emulator for your machine. If you're running Windows, you can currently download executables from the project's home page at http://simh.trailing-edge.com. If you're running Mac OS-X, you may be able to download the executables; try a Google search, but of course be cautious when running executables from an unknown source. Linux users and users of non-mainstream systems will probably need to download the source from the SIMH home page and compile themselves. (Windows and Mac users of course also have the option of doing this.) In any case, you ONLY need the PDP-10 emulator. Place this program file (called "pdp10.exe" under Windows) in the same directory where you stored the contents of this zip file.

STARTING YOUR VIRTUAL PDP-10
============================

1. Bring up a command prompt in the PDP-10 In a Box directory using whatever technique is normal for your platform.

2. Enter "pdp10 tops10.cfg".

3. At the "BOOT>" prompt, just press enter.

4. At the "Why reload:" prompt, enter "NEW".

5. At the "Date:" prompt, just press enter to accept your host computer's current date. If you wish, you may enter another date in the format "MM-DD-YYYY".

6. At the "Time:" prompt, just press enter to accept your host computer's current time. If you wish, you may enter another time in the format "HHMMSS".

7. At the "Startup option:" prompt, enter "GO".

8. After a moment, the system will place you in the system operator's console (denoted by the "OPR>" prompt). To work with programs and files like a normal user, just type "EXIT" here.

SHUTTING DOWN YOUR VIRTUAL PDP-10
=================================

To keep your virtual OS clean and uncorrupted, I strongly recommend that you always take a moment to shut your virtual PDP-10 down properly when you are finished working with it.

1. From the normal user prompt, enter "R OPR" to return to the system operator's console.

2. Enter "SET KSYS NOW".

3. Enter "EXIT" to return to the normal user prompt.

4. Enter "KJOB".

5. It is now safe to close the window.

RUNNING ADVENTURE
=================

There are two versions of Adventure in your Home directory, which you can view by entering the command "DIR". The files you'll see there are as follows:

ADV.F4: FORTRAN source of Will Crowther's original (and very incomplete) Adventure, probably abandoned by him in early 1976.
ADV.DAT: The data file used by the above.
ADV.EXE: Executable of Crowther's Adventure.
ADV.REL: Object file of Crowther's Adventure created by the FORTRAN compiler.
ADVENT.F4: FORTRAN source of the "real" Adventure we all came to know and love, as expanded, completed, and perfected by Don Woods in early 1977.
ADVENT.DAT: The data file used by the above.
ADVENT.EXE: Executable of the completed Adventure.
ADVENT.REL: Object file of the completed Adventure created by the FORTRAN compiler.
IOFIL.FOR: FORTRAN source of an I/O utility function used by both versions of Adventure.
IOFIL.REL: Object file of the same.

To play Crowther's Adventure, you need only type "RUN ADV".
To play the completed Adventure, type "RUN ADVENT".

SAVING YOUR GAME
================

Crowther's Adventure has no save ability. The completed version does, but it works in a somewhat odd way.

You can save by typing "SAVE" at any in-game prompt, at which point the game will dump you back to the command line with instructions to "BE SURE TO SAVE YOUR CORE IMAGE." This is not an idle suggestion. To complete the saving process, you MUST type "SAVE <filename>" at the "." prompt before doing anything else. For instance, you could type "SAVE A1" to save your current game under the name "A1". To return to a saved game, you simply need to "RUN <your saved filename>" rather than the original "ADVENT" program.

Be aware that TOPS-10 filenames are automatically truncated to 6 characters. If you were to "SAVE ADVENT1", you would actually overwrite your original "ADVENT" game. (Although you could of course always recompile it; see below.)

There's also that annoying restriction that you cannot resume a saved game until 90 minutes have passed. You can get around that by restarting the system and monkeying with the time (see above) if you really want / need to.


CAVE HOURS AND MAGIC MODE
=========================

Because TOPS-10 is a time-shared OS normally shared by many users, the completed version of Adventure will by default allow only a short "preview" play on weekdays between the hours of 8:00 and 18:00, thus conserving resources for more serious work. However, the compiled version included with this distribution has been configured to allow unfettered 24-hour access. You can change that by entering magic mode, like this:

1. Enter "MAGIC MODE" at any normal in-game prompt.

2. To "ARE YOU A WIZARD?" reply "YES".

3. To "PROVE IT! SAY THE MAGIC WORD!" reply "DWARF".

4. To "DO YOU KNOW WHAT I THOUGHT IT WAS?" reply "NO".

5. The game will reply with a group of five letters. Your answer depends upon not only these letters but the exact system time and the "Magic Number," which is currently set to its default of 11111. You can learn what your reply should be in one of two ways. 1) Navigate with a web browser to http://www.zonadepruebas.com/magicmode_crack_en.html and fill out the interactive form there. 2) Use the included Python script (advent-magic-mode.py) kindly donated by Grawity (grawity@gmail.com); to use this method, you will of course need to have the Python scripting language installed on your system. If you get it right -- and yes, the timing is tricky -- you will be allowed to change not only the "Cave Hours" but also the opening message, the "Magic Word," the "Magic Number," and more.

6. After you are done and have exited the game, you MUST save your core image for your changes to take effect. Type "SAVE ADVENT" at the "." prompt, or give your new version another name if you'd like.

COMPILING ADVENTURE
===================

While details of PDP-10 development are both out of the scope of this project and (for the most part) out of the scope of my knowledge, know that you can compile either version of ADVENT by entering "LOAD <Adventure source file>, IOFIL.FOR" at the "." prompt. Once that is completed, you can save the compiled executable by entering "SAVE <executable filename>".

The "SOS" editor is included with this distribution. You can use it to edit the source files. I'm afraid you're one your with that one, though.

BASIC
=====

You can start the interactive BASIC environment by typing "R BASIC" at the "." prompt. Type "SYSTEM" from there to get back out.


OTHER RESOURCES
===============

See the Bit Savers PDP-10 document repository at http://www.bitsavers.org/pdf/dec/pdp10/ for a treasure trove of original PDP-10 and TOPS-10 documentation. The Usenet and Google Group alt.sys.pdp10 is probably the best place to ask specific questions.

Questions and suggestions directed to me are welcome, but please be aware that I'm not an expert on the PDP-10 or TOPS-10, just someone with a strong interest in the history of computing, particularly computer gaming. You can reach me at maher@filfre.net. My blog can be found at http://www.filfre.net.

Jimmy Maher
May 20, 2011

