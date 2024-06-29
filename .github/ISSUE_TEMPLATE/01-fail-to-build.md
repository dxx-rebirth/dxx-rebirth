---
name: Failure to build
about: Report a failure to build current code in any supported configuration.
labels: 'build-failure'
title: 'Build failure: '
assignees: 'vLKp'

---

<!--
These instructions are wrapped in comment markers.  Write your answers outside the comment markers.  You may delete the commented text as you go, or leave it in and let the system remove the comments when you submit the issue.

Use this template if the code in master fails to build for you.  If your problem happens at runtime, please use the issue template `Runtime crash` or the issue template `Runtime bug`, as appropriate.

Please use a descriptive title.  Include in the title the _first_ error message.  Attach the full output of `scons` or paste it inline inside triple backticks.  Collect the output from a run with `verbosebuild=1`; `verbosebuild=1` is default-enabled when output is written to a file or can be set explicitly on the command line.
-->
### Environment

<!--
If you fetched the source from Git, state the Git commit you used, preferably as the full 40-digit commit hash.  Please do **not** say "HEAD", "current", or similar relative references.  The meaning of relative references can change as contributors publish new code.  The 40-digit commit hash will not change.

If you received a pre-packaged source archive from someone, describe how others can get the same archive.  For publicly linked downloads, the download URL of the archive is sufficient.  Please link to the program archive, not to the web page which links to the program archive.

  Good URL: https://www.dxx-rebirth.com/download/dxx/user/afuturepilot/dxx-rebirth_v0.60-weekly-04-14-18-win.zip
  Bad URL: https://www.dxx-rebirth.com/download-dxx-rebirth/
-->

#### Operating System Environment

<!--
State what host platform (Microsoft Windows, Mac OS X, or Linux, *BSD) you tried.  If you tried multiple, list all of them.
-->

* [ ] Microsoft Windows (32-bit)
* [ ] Microsoft Windows (64-bit)
* [ ] Mac OS X
<!--
* For Linux, give the name of the distribution.
** For distributions with specific releases (Debian, Fedora, Ubuntu), give the name and number of the release.
** For rolling distributions (Arch, Gentoo), describe how recently the system was fully updated.  Reports from out-of-date systems are not rejected.  However, if your issue is known to be fixed by a particular update, the Rebirth maintainers may suggest that update instead of changing Rebirth.

Add versions as needed.
-->

* Debian
  * [ ] Debian Bookworm
  * [ ] Debian Trixie
  * [ ] Debian Sid
* Fedora
  * [ ] Fedora 36
  * [ ] Fedora 37
  * [ ] Fedora 38
  * [ ] Fedora 39
  * [ ] Fedora 40
  * [ ] Rawhide
* Ubuntu
  * [ ] Ubuntu 20.04 LTS (Focal Fossa)
  * [ ] Ubuntu 22.04 LTS (Jammy Jellyfish)
  * [ ] Ubuntu 23.10 (Mantic Minotaur)
  * [ ] Ubuntu 24.04 LTS (Noble Numbat)

* [ ] Arch
* [ ] Gentoo

* [ ] OpenBSD
* [ ] FreeBSD
* [ ] NetBSD

#### CPU environment

<!--
Indicate which CPU families were targeted.  Some bugs are only visible on certain architectures, since other architectures hide the consequences of the mistake.
If unsure, omit this section.  Generally, if you are on an architecture that requires special consideration, you will know your architecture.
-->
* [ ] x86 (32-bit Intel/AMD)
* [ ] x86\_64 (64-bit Intel/AMD)
* [ ] ARM (32-bit)
* [ ] ARM64 (64-bit; sometimes called AArch64)
* [ ] PowerPC (32-bit, sometimes called ppc)
* [ ] PowerPC64 (64-bit, big-endian, sometimes called ppc64)
* [ ] PowerPC64Le (64-bit, little-endian, sometimes called ppc64le)

### Description

<!--
Describe the issue here.
-->

#### Regression status

<!--
What is the oldest Git commit known to present the problem?  What is the newest Git commit known not to present the problem?  Ideally, the newest unaffected is an immediate parent of the oldest affected.  However, if the reporter lacks the ability to test individual versions (or the time to do so), there may be a range of untested commits for which the affected/unaffected status is unknown.  Reports are not rejected due to a wide range of untested commits.  However, smaller commit ranges are often easier to debug, so better information here improves the chance of a quick resolution.
-->

### Steps to Reproduce

<!--
For build failures, provide:
- The `scons` command executed.
- The contents of `site-local.py`, if present.
- All output from `scons`, starting at the prompt where the command was entered and ending at the first shell prompt after the error.
- If sconf.log is mentioned in the output, attach it.  If it is mentioned, it will be in the last lines before SCons exits.  You do not need to read the full output searching for references to it.  If in doubt, attach it.
- If `dxxsconf.h` is generated, attach it.  It will be in the root of the build directory.  If you did not set a build directory, it will be in the same directory as `SConstruct`.
-->
