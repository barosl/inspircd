/*       +------------------------------------+ *       | Inspire Internet Relay Chat Daemon | *       +------------------------------------+ * *  InspIRCd is copyright (C) 2002-2007 ChatSpike-Dev. *		       E-mail: *		<brain@chatspike.net> *		<Craig@chatspike.net> * * Written by Craig Edwards, Craig McLure, and others. * This program is free but copyrighted software; see *	    the file COPYING for details. * * --------------------------------------------------- */#ifndef __CMD_LINKS_H__#define __CMD_LINKS_H__// include the common header files#include "users.h"#include "channels.h"/** Handle /LINKS. These command handlers can be reloaded by the core, * and handle basic RFC1459 commands. Commands within modules work * the same way, however, they can be fully unloaded, where these * may not. */class cmd_links : public command_t{ public:	/** Constructor for links.	 */	cmd_links (InspIRCd* Instance) : command_t(Instance,"LINKS",0,0) { }	/** Handle command.	 * @param parameters The parameters to the comamnd	 * @param pcnt The number of parameters passed to teh command	 * @param user The user issuing the command	 * @return A value from CmdResult to indicate command success or failure.	 */	CmdResult Handle(const char** parameters, int pcnt, userrec *user);};#endif