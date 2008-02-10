/***************************************************************************
 * statefile.c:
 *
 * Routines to save and recover DataLink state information to/from a file.
 *
 * Written by Chad Trabant, IRIS Data Management Center
 *
 * modified: 2008.040
 ***************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "libdali.h"


/***************************************************************************
 * dl_savestate:
 *
 * Save the all the current the sequence numbers and time stamps into the
 * given state file.
 *
 * Returns:
 * -1 : error
 *  0 : completed successfully
 ***************************************************************************/
int
dl_savestate (DLCP *dlconn, const char *statefile)
{
  char line[200];
  int linelen;
  int statefd;
  
  if ( ! dlconn || ! statefile )
    return -1;

  /* Open the state file */
  if ( (statefd = dlp_openfile (statefile, 'w')) < 0 )
    {
      dl_log_r (dlconn, 2, 0, "cannot open state file for writing\n");
      return -1;
    }
  
  dl_log_r (dlconn, 1, 2, "saving connection state to state file\n");
  
  /* Write state information: <server address> <packet ID> <packet time> */
  linelen = snprintf (line, sizeof(line), "%s %lld %lld\n",
		      dlconn->addr, dlconn->pktid, dlconn->pkttime);
  
  if ( write (statefd, line, linelen) != linelen )
    {
      dl_log_r (dlconn, 2, 0, "cannot write to state file, %s\n", strerror (errno));
      return -1;
    }
  
  if ( close (statefd) )
    {
      dl_log_r (dlconn, 2, 0, "cannot close state file, %s\n", strerror (errno));
      return -1;
    }
  
  return 0;
} /* End of dl_savestate() */


/***************************************************************************
 * dl_recoverstate:
 *
 * Recover connection state from a state file.
 *
 * Returns:
 * -1 : error
 *  0 : completed successfully
 *  1 : file could not be opened (probably not found)
 ***************************************************************************/
int
dl_recoverstate (DLCP *dlconn, const char *statefile)
{
  int statefd;
  char line[200];
  char addrstr[100];
  int64_t pktid;
  dltime_t pkttime;
  int fields;
  int found = 0;
  int count = 1;
  
  if ( ! dlconn || ! statefile )
    return -1;
  
  /* Open the state file */
  if ( (statefd = dlp_openfile (statefile, 'r')) < 0 )
    {
      if ( errno == ENOENT )
	{
	  dl_log_r (dlconn, 1, 0, "could not find state file: %s\n", statefile);
	  return 1;
	}
      else
	{
	  dl_log_r (dlconn, 2, 0, "could not open state file, %s\n", strerror (errno));
	  return -1;
	}
    }
  
  dl_log_r (dlconn, 1, 1, "recovering connection state from state file\n");
  
  /* Loop through lines in the file and find the matching server address */
  while ( (dl_readline (statefd, line, sizeof(line))) >= 0 )
    {
      addrstr[0] = '\0';
      
      fields = sscanf (line, "%s %lld %lld\n", addrstr, &pktid, &pkttime);
      
      if ( fields < 0 )
        continue;
      
      if ( fields < 3 )
	{
	  dl_log_r (dlconn, 2, 0, "could not parse line %d of state file\n", count);
	}
      
      /* Check for a matching server address and set connection values if found */
      if ( ! strncmp (dlconn->addr, addrstr, sizeof(addrstr)) )
	{
	  dlconn->pktid = pktid;
	  dlconn->pkttime = pkttime;
	  
	  found = 1;
	  break;
	}
      
      count++;
    }
  
  if ( ! found )
    {
      dl_log_r (dlconn, 1, 0, "Server address not found in state file: %s\n", dlconn->addr);
    }  
  
  if ( close (statefd) )
    {
      dl_log_r (dlconn, 2, 0, "could not close state file, %s\n", strerror (errno));
      return -1;
    }

  return 0;
} /* End of dl_recoverstate() */
