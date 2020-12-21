// I'm not a java coder at all and having some way of automatic testing is
// always a good thing. So I've copied some parts of rxtx to here to allow us
// to do some "fake locking" with "make test" to verify our preload does its job
//
// Sidenote: Yes, this is over 400 lines of code just for locking a device...

#define DEVICEDIR "/dev/"
#define LOCKDIR "/var/lock"
#define LOCKFILEPREFIX "LCK.."
#define UNEXPECTED_LOCK_FILE "RXTX Error:  Unexpected lock file: %s\n Please report to the RXTX developers\n"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <signal.h>


void report(char* message) {
  fprintf(stderr, message);
}
#define report_error report
#define report_warning report


/*----------------------------------------------------------
 check_group_uucp

   accept:     none
   perform:    check if the user is root or in group uucp
   return:     0 on success
   exceptions: none
   comments:
		This checks if the effective user is in group uucp so we can
		create lock files.  If not we give them a warning and bail.
		If its root we just skip the test.

		if someone really wants to override this they can use the			USER_LOCK_DIRECTORY --not recommended.

		In a recent change RedHat 7.2 decided to use group lock.
		In order to get around this we just check the group id
		of the lock directory.

		* Modified to support Debian *

		The problem was that checking the ownership of the lock file
		dir is not enough, in the sense that even if the current user
		is not in the group of the lock directory if the lock
		directory has 777 permissions the lock file can be anyway
		created.  My solution is simply to try to create a tmp file
		there and if it works then we can go on.  Here is my code that
		I tried and seems to work.

		Villa Valerio <valerio.villa@siemens.com>
----------------------------------------------------------*/
int check_group_uucp()
{
	FILE *testLockFile ;
	char testLockFileDirName[] = LOCKDIR;
	char testLockFileName[] = "tmpXXXXXX";
	char *testLockAbsFileName;

	testLockAbsFileName = calloc(strlen(testLockFileDirName)
			+ strlen(testLockFileName) + 2, sizeof(char));
	if ( NULL == testLockAbsFileName )
	{
		report_error("check_group_uucp(): Insufficient memory\n");
		return 1;
	}
	strcat(testLockAbsFileName, testLockFileDirName);
	strcat(testLockAbsFileName, "/");
	strcat(testLockAbsFileName, testLockFileName);
	if ( NULL == mktemp(testLockAbsFileName) )
	{
		free(testLockAbsFileName);
		report_error("check_group_uucp(): mktemp malformed string - \
			should not happen\n");

		return 1;
	}
	testLockFile = fopen (testLockAbsFileName, "w+");
	if (NULL == testLockFile)
	{
		report_error("check_group_uucp(): error testing lock file "
			"creation Error details:");
		report_error(strerror(errno));
      report_error("\n");
		free(testLockAbsFileName);
		return 1;
	}

	fclose (testLockFile);
	unlink (testLockAbsFileName);
	free(testLockAbsFileName);

	return 0;
}


/*----------------------------------------------------------
 is_device_locked

   accept:      char * filename.  The device in question including the path.
   perform:     see if one of the many possible lock files is aready there
		if there is a stale lock, remove it.
   return:      1 if the device is locked or somethings wrong.
		0 if its possible to create our own lock file.
   exceptions:  none
   comments:    check if the device is already locked
----------------------------------------------------------*/
int is_device_locked( const char *port_filename )
{
	const char *lockdirs[] = { "/etc/locks", "/usr/spool/kermit",
		"/usr/spool/locks", "/usr/spool/uucp", "/usr/spool/uucp/",
		"/usr/spool/uucp/LCK", "/var/lock", "/var/lock/modem",
		"/var/spool/lock", "/var/spool/locks", "/var/spool/uucp",
		LOCKDIR, NULL
	};
	const char *lockprefixes[] = { "LCK..", "lk..", "LK.", NULL };
	char *p, file[80], pid_buffer[20], message[80];
	int i = 0, j, k, fd , pid;
	struct stat buf, buf2, lockbuf;

	j = strlen( port_filename );
	p = ( char * ) port_filename+j;
	while( *( p-1 ) != '/' && j-- !=1 ) p--;

	stat(LOCKDIR, &lockbuf);
	while( lockdirs[i] )
	{
		/*
		   Look for lockfiles in all known places other than the
		   defined lock directory for this system
		   report any unexpected lockfiles.

		   Is the suspect lockdir there?
		   if it is there is it not the expected lock dir?
		*/
		if( !stat( lockdirs[i], &buf2 ) &&
			buf2.st_ino != lockbuf.st_ino &&
			strncmp( lockdirs[i], LOCKDIR, strlen( lockdirs[i] ) ) )
		{
			j = strlen( port_filename );
			p = ( char *  ) port_filename + j;
			while( *( p - 1 ) != '/' && j-- != 1 )
			{
				p--;
			}
			k=0;
			while ( lockprefixes[k] )
			{
				/* FHS style */
				sprintf( file, "%s/%s%s", lockdirs[i],
					lockprefixes[k], p );
				if( stat( file, &buf ) == 0 )
				{
					sprintf( message, UNEXPECTED_LOCK_FILE,
						file );
					report_warning( message );
					return 1;
				}

				/* UUCP style */
				stat(port_filename , &buf );
				sprintf( file, "%s/%s%03d.%03d.%03d",
					lockdirs[i],
					lockprefixes[k],
					(int) major( buf.st_dev ),
					(int) major( buf.st_rdev ),
					(int) minor( buf.st_rdev )
				);
				if( stat( file, &buf ) == 0 )
				{
					sprintf( message, UNEXPECTED_LOCK_FILE,
						file );
					report_warning( message );
					return 1;
				}
				k++;
			}
		}
		i++;
	}

	/*
		OK.  We think there are no unexpect lock files for this device
		Lets see if there any stale lock files that need to be
		removed.
	*/

	/*  FHS standard locks */
	i = strlen( port_filename );
	p = ( char * ) port_filename + i;
	while( *(p-1) != '/' && i-- != 1)
	{
		p--;
	}
	sprintf( file, "%s/%s%s", LOCKDIR, LOCKFILEPREFIX, p );

	if( stat( file, &buf ) == 0 )
	{
		/* check if its a stale lock */
		fd=open( file, O_RDONLY );
		read( fd, pid_buffer, 11 );
		/* FIXME null terminiate pid_buffer? need to check in Solaris */
		close( fd );
		sscanf( pid_buffer, "%d", &pid );

		if( kill( (pid_t) pid, 0 ) && errno==ESRCH )
		{
			sprintf( message,
				"RXTX Warning:  Removing stale lock file. %s\n",
				file );
			report_warning( message );
			if( unlink( file ) != 0 )
			{
				snprintf( message, 80, "RXTX Error:  Unable to \
					remove stale lock file: %s\n",
					file
				);
				report_warning( message );
				return 1;
			}
		}
	}
	return 0;
}


/*----------------------------------------------------------
 check_lock_status

   accept:      the lock name in question
   perform:     Make sure everything is sane
   return:      0 on success
   exceptions:  none
   comments:
----------------------------------------------------------*/
int check_lock_status( const char *filename )
{
	struct stat buf;
	/*  First, can we find the directory? */

	if ( stat( LOCKDIR, &buf ) != 0 )
	{
		report( "check_lock_status: could not find lock directory.\n" );
		return 1;
	}

	/*  OK.  Are we able to write to it?  If not lets bail */

	if ( check_group_uucp() )
	{
		report_error( "check_lock_status: No permission to create lock file.\nplease see: How can I use Lock Files with rxtx? in INSTALL\n" );
		return(1);
	}

	/* is the device alread locked */

	if ( is_device_locked( filename ) )
	{
		report( "check_lock_status: device is locked by another application\n" );
		return 1;
	}
	return 0;

}


/*----------------------------------------------------------
 fhs_lock

   accept:      The name of the device to try to lock
                termios struct
   perform:     Create a lock file if there is not one already.
   return:      1 on failure 0 on success
   exceptions:  none
   comments:    This is for linux and freebsd only currently.  I see SVR4 does
                this differently and there are other proposed changes to the
		Filesystem Hierachy Standard

		more reading:

----------------------------------------------------------*/
int fhs_lock( const char *filename, int pid )
{
	/*
	 * There is a zoo of lockdir possibilities
	 * Its possible to check for stale processes with most of them.
	 * for now we will just check for the lockfile on most
	 * Problem lockfiles will be dealt with.  Some may not even be in use.
	 *
	 */
	int fd,j;
	char lockinfo[12], message[80];
	char file[80], *p;

	j = strlen( filename );
	p = ( char * ) filename + j;
	/*  FIXME  need to handle subdirectories /dev/cua/...
	    SCO Unix use lowercase all the time
			taj
	*/
	while( *( p - 1 ) != '/' && j-- != 1 )
	{
		p--;
	}
	sprintf( file, "%s/LCK..%s", LOCKDIR, p );
	if ( check_lock_status( filename ) )
	{
		report( "fhs_lock() lockstatus fail\n" );
		return 1;
	}
	fd = open( file, O_CREAT | O_WRONLY | O_EXCL, 0444 );
	if( fd < 0 )
	{
		sprintf( message,
			"RXTX fhs_lock() Error: creating lock file: %s: %s\n",
			file, strerror(errno) );
		report_error( message );
		return 1;
	}
	sprintf( lockinfo, "%10d\n",(int) getpid() );
	sprintf( message, "fhs_lock: creating lockfile: %s", lockinfo );
	report( message );
	write( fd, lockinfo, 11 );
	close( fd );
	return 0;
}

/*----------------------------------------------------------
 check_lock_pid

   accept:     the name of the lockfile
   perform:    make sure the lock file is ours.
   return:     0 on success
   exceptions: none
   comments:
----------------------------------------------------------*/
int check_lock_pid( const char *file, int openpid )
{
	int fd, lockpid;
	char pid_buffer[12];
	char message[80];

	fd=open( file, O_RDONLY );
	if ( fd < 0 )
	{
		return( 1 );
	}
	if ( read( fd, pid_buffer, 11 ) < 0 )
	{
		close( fd );
		return( 1 );
	}
	close( fd );
	pid_buffer[11] = '\0';
	lockpid = atol( pid_buffer );
	/* Native threads JVM's have multiple pids */
	if ( lockpid != getpid() && lockpid != getppid() && lockpid != openpid )
	{
		sprintf(message, "check_lock_pid: lock = %s pid = %i gpid=%i openpid=%i\n",
			pid_buffer, (int) getpid(), (int) getppid(), openpid );
		report( message );
		return( 1 );
	}
	return( 0 );
}


/*----------------------------------------------------------
 fhs_unlock

   accept:      The name of the device to unlock
   perform:     delete the lock file
   return:      none
   exceptions:  none
   comments:    This is for linux only currently.  I see SVR4 does this
                differently and there are other proposed changes to the
		Filesystem Hierachy Standard
----------------------------------------------------------*/
int fhs_unlock( const char *filename, int openpid )
{
	char file[80],*p;
	int i;

	i = strlen( filename );
	p = ( char * ) filename + i;
	/*  FIXME  need to handle subdirectories /dev/cua/... */
	while( *( p - 1 ) != '/' && i-- != 1 ) p--;
	sprintf( file, "%s/LCK..%s", LOCKDIR, p );

	if( !check_lock_pid( file, openpid ) )
	{
		unlink(file);
		report("fhs_unlock: Removing LockFile\n");
	}
	else
	{
		report("fhs_unlock: Unable to remove LockFile\n");
      return 1;
	}

   return 0;
}





int main (int argc, char *argv[]) {
  char* dev = "/dev/ttyS3";
  printf("Testing lock on %s: ", dev);
  int i = fhs_lock(dev, getpid());
  if (i == 0)
    printf("PASS\n");
  else {
    printf("FAIL with status %d\n", i);
    exit(1);
  }

  printf("Testing unlock on %s: ", dev);
  i = fhs_unlock(dev, getpid());
  if (i == 0)
    printf("PASS\n");
  else {
    printf("FAIL with status %d\n", i);
    exit(1);
  }

  exit(0);
}
