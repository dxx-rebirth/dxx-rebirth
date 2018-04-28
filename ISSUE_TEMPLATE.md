<!--
These instructions are wrapped in comment markers.  Write your answers outside the comment markers.  You may delete the commented text as you go, or leave it in and let the system remove the comments when you submit the issue.

Please use a descriptive title.  If you are reporting a build failure, start the title with "Build failure:" and include in the title the _first_ error message.  If you are reporting a program crash, start the title with "Crash:" and summarize your actions immediately prior to the crash.

For your convenience, various multiple choice queries include many of the common answers with a leading `[ ]`.  For each possible answer, if the issue was tested and applies, change `[ ]` to `[x]`.  If the issue was tested and did not apply, leave `[ ]` unchanged.  If the issue was not tested, remove the line.  You are not expected to test every combination.  In many cases, you are likely to delete more possible answers as untested than you keep.

If you need to include program output inline (build log snippets, gamelog, etc.), wrap single-line output in `backticks` and wrap multi-line output in
```
triple backticks
```
For triple backticks, place the backticks alone on a line.
-->
### Environment

<!--
If you fetched the source from Git, state the Git commit you used, preferably as the full 40-digit commit hash.  Please do **not** say "HEAD", "current", or similar relative references.  The meaning of relative references can change as contributors publish new code.  The 40-digit commit hash will not change.

If you received a pre-compiled program from someone, describe how others can get the same program.  For publicly linked downloads, the download URL of the pre-compiled program is sufficient.  Please link to the program archive, not to the web page which links to the program archive.

  Good URL: https://www.dxx-rebirth.com/download/dxx/user/afuturepilot/dxx-rebirth_v0.60-weekly-04-14-18-win.zip
  Bad URL: https://www.dxx-rebirth.com/download-dxx-rebirth/
-->

#### Operating System Environment

<!--
State what platform (Microsoft Windows, Mac OS X, or Linux, *BSD) you used.  If you used multiple, list all of them.
-->

<!--
For Windows, if readily available, also state the installed Service Pack.
-->
* Microsoft Windows
  * [ ] Microsoft Windows XP
  * [ ] Microsoft Windows Vista
  * [ ] Microsoft Windows 7
  * [ ] Microsoft Windows 8
  * [ ] Microsoft Windows 8.1
  * [ ] Microsoft Windows 10

<!--
Mac OS X.  Add versions as needed.
-->
* Mac OS X
  * [ ] Mac OS X 10.8
  * [ ] Mac OS X 10.9
  * [ ] Mac OS X 10.10
  * [ ] Mac OS X 10.11
  * [ ] Mac OS X 10.12
  * [ ] Mac OS X 10.13

<!--
* For Linux, give the name of the distribution.
** For distributions with specific releases (Debian, Fedora, Ubuntu), give the name and number of the release.
** For rolling distributions (Arch, Gentoo), describe how recently the system was fully updated.  Reports from out-of-date systems are not rejected.  However, if your issue is known to be fixed by a particular update, the Rebirth maintainers may suggest that update instead of changing Rebirth.

Add versions as needed.
-->

* Debian
  * [ ] Debian Wheezy
  * [ ] Debian Jessie
  * [ ] Debian Stretch
  * [ ] Debian Buster
* Fedora
  * [ ] Fedora 26
  * [ ] Fedora 27
  * [ ] Fedora 28
  * [ ] Fedora 29
* Ubuntu
  * [ ] Ubuntu 16.04 LTS (Xenial Xerus)
  * [ ] Ubuntu 16.10 (Yakkety Yak)
  * [ ] Ubuntu 17.04 (Zesty Zapus)
  * [ ] Ubuntu 17.10 (Artful Aardvark)
  * [ ] Ubuntu 18.04 LTS (Bionic Beaver)

* [ ] Arch
* [ ] Gentoo

* [ ] OpenBSD

#### CPU environment

<!--
Indicate which CPU families were tested for the issue.  Some bugs are only visible on certain architectures, since other architectures hide the consequences of the mistake.
If unsure, omit this section.  Generally, if you are on an architecture that requires special consideration, you will know your architecture.
-->
* [ ] x86 (32-bit Intel/AMD)
* [ ] x86\_64 (64-bit Intel/AMD)
* [ ] ARM (32-bit)
* [ ] ARM64 (64-bit; sometimes called AArch64)

#### Game environment

<!--
If the issue is specific to a particular mission, give the name of the campaign and the level of the mission within that campaign.  If the campaign is not one of the core assets (`Descent: First Strike`, `Descent 2: Counterstrike`, or `Descent 2: Vertigo`), give a download link to the campaign.

If the issue occurs at some particular place in the level, give a description how to reach that point from the beginning of the level.  Assume that the maintainer can use cheats to acquire keys, skip difficult fights, etc., but that the maintainer is not familiar with the optimal route to get from the start point to the affected location.

Regardless of whether the mission is a builtin campaign or custom campaign, identify the version of the Descent or Descent 2 assets you used.  Some issues have impacted only specific versions of the game data.  The simplest way to identify the asset is to report the size in bytes of `descent.hog` or `descent2.hog`, as appropriate.
-->

### Description

<!--
Describe the issue here.
-->

#### Regression status

<!--
Is the reported problem present in prior releases of Rebirth?  Is it a bug from the original game?

What is the oldest Git commit known to present the problem?  What is the newest Git commit known not to present the problem?  Ideally, the newest unaffected is an immediate parent of the oldest affected.  However, if the reporter lacks the ability to test individual versions (or the time to do so), there may be a range of untested commits for which the affected/unaffected status is unknown.  Reports are not rejected due to a wide range of untested commits.  However, smaller commit ranges are often easier to debug, so better information here improves the chance of a quick resolution.
-->

### Steps to Reproduce

<!--
For build failures, provide:
- The `scons` command executed.
- All output from `scons`, starting at the prompt where the command was entered and ending at the first shell prompt after the error.
- If sconf.log is mentioned in the output, attach it.  If it is mentioned, it will be in the last lines before SCons exits.  You do not need to read the full output searching for references to it.  If in doubt, attach it.
- If `dxxsconf.h` is generated, attach it.  It will be in the root of the build directory.  If you did not set a build directory, it will be in the same directory as `SConstruct`.

For runtime problems (crashes, hangs, incorrect results), provide:
- Expected behavior
- Observed behavior
- Engines affected (D1X-Rebirth, D2X-Rebirth)
- Steps, starting from the main menu, to reach the problem state.  Assume the maintainer can cheat to any level and knows Descent input controls, but is unfamiliar with the particular level.
- If possible, describe the frequency of the problem.  Does it happen every time the steps to reproduce are followed?  If it is intermittent, are there any events correlated with the error?
- If the game produced any error messages, include their text verbatim.  If you paraphrase the message, you will likely be asked to reproduce the error and collect a verbatim copy of the text.
- For in-game problems, indicate whether it happens in single player, multiplayer cooperative, or multiplayer competitive.  If you do not know, state that.  You do not need to check every combination before filing, but please report which combinations you checked and the results you found for those combinations.
- If the game crashed and a crash dump was created, include the dump backtrace.
-->

* Engines affected:
  * [ ] D1X-Rebirth
  * [ ] D2X-Rebirth

* Game modes affected:
  * [ ] Single player
  * [ ] Multiplayer competitive
    * [ ] Anarchy
    * [ ] Team Anarchy
    * [ ] Robo-Anarchy
    * [ ] Capture the Flag
    * [ ] Bounty
  * [ ] Multiplayer cooperative

<!--
If the issue is only observed in single player, delete this next group.
-->
* Players affected by issue:
  * [ ] Game host
  * [ ] Game guests
