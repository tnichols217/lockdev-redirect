# lockdev-redirect

lockdev-redirect is helper which redirects /var/lock (and /run/lock) for a given application to a (safe) user-writable directory below $XDG_RUNTIME_DIR. This allows to make software work which expects write access to /var/lock without weakening system security by actually allowing this write access. The main target are closed-source applications which can't be easily updated to use "flock" for device locking instead of obsolete uucp device locking.

Common error messages you may run into (and can be fixed with lockdev-redirect):

```
check_group_uucp(): error testing lock file creation
Error details:Permission denied
check_lock_status: No permission to create lock file.
please see: How can I use Lock Files with rxtx? in INSTALL
```
```
Unable to create lock file for the port. Permission denied
Error connecting to the serial port.
```

## Compilation and installation

To compile run

```
make
```

To install the resulting library and the lockdev-redirect launcher, run

```
sudo make install
```

## Usage

Just prepend "lockdev-redirect" to the command line of whatever application you want to run with redirected /var/lock

```bash
lockdev-redirect /path/to/app --whatever-param=something
```

## Performing tests

After compiling you can run some tests with

```
make test
```

This runs some actual device locking routines that are used by existing libraries where I ran into permission problems with.

## Reporting errors

It may be possible that you still run into errors with some applications. The redirect only has been implemented for glibc functions that are used by known uucp lock implementations.
If you run into errors, then please provide the following in your issue report:

 - Name of the application you wanted to run with lockdev-redirect
 - Library used for device access or device locking (if known)
 - Download path or website so I can get this application
 - If this application is not commonly available, you also have to bring some time to help with debugging

## Please read before attempting to add new overrides

The path /var/lock, that we want to redirect, is a very special path which should only be used by device locking mechanisms. What these mechanisms do is actually pretty limited so we don't have to provide a nearly perfect "virtual mount" of the new lock path to /var/lock but a subset of the functions actually used by locking mechanisms is perfectly fine for our usecase. Sometimes less is more!

So please:

 - Don't start to try to add as many function overrides as possible! We don't want everything but the kitchen sink in there!
 - Only attempt to override more functions if you can provide a real-world example where this is actually needed to make a new device-locking mechanism work.
 - Always keep in mind that we need a working testcase for every new override. Best case this would be code from the actual application (if it is open source).

## My suggestion to application developers

lockdev-redirect is meant as a helper for the **end user only** and is not meant to be used as a replacement of implementing proper (up-to-date) device locking. So I strongly suggest against delivering lockdev-redirect bundled with closed source applications. The up-to-date replacement for uucp lock files is using "[flock](https://linux.die.net/man/2/flock)" on the open file descriptor of the device. You can find some additional information about this in the following Debian bug report, where they deprecate their own uucp lock helper library:
https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=734086

Unfortunately Debian still [kept world-writable /var/lock for legacy reasons](https://salsa.debian.org/systemd-team/systemd/-/blob/debian/master/debian/patches/debian/Make-run-lock-tmpfs-an-API-fs.patch), but that's a different story...

## Why /var/lock for device locking never was a good idea

I want to finish this README.md with some personal thoughts about /var/lock device locking that I came up with while digging into the whole problem.

### Security issues

There at least was one security issue where world-writable /var/lock was involved:
https://systemd-devel.freedesktop.narkive.com/vCJLfMo2/headsup-var-lock-and-var-lock-lockdev
I don't know if this is still an issue nowadays, but if users have write access to a directory where daemons, running as the "root" user, place their files, then this is just an security issue waiting to happen.

Just remember: Debian still has /var/lock (or /run/lock) world-writable!

### Global serial device dead lock

To make it even clearer how bad this "world writable lock files" idea actually is: This kind of lock does not depend at all on permission to access the actual devices! So in fact you can lock every device on the system with every user. No actual access to the device needed! All that is needed to lock out other device users is some shell script snippet like this:

```bash
for file in /dev/tty*; do
  echo "         1" > "/var/lock/LCK..$(basename $file)"
done
```

What this does is create lock files for every serial device currently available on the system. It places PID "1" as the process using the device which is the system init process. This process exists for as long as the system runs. The result is that no application, using uucp locks, will access this device as they expect it to be locked by a still-running process.

The only reason why this no longer causes any harm is that (fortunately) more and more applications are updated to no longer use this kind of device locking.
