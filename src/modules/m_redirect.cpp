/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd is copyright (C) 2002-2006 ChatSpike-Dev.
 *		       E-mail:
 *		<brain@chatspike.net>
 *	   	  <Craig@chatspike.net>
 *     
 * Written by Craig Edwards, Craig McLure, and others.
 * This program is free but copyrighted software; see
 *	    the file COPYING for details.
 *
 * ---------------------------------------------------
 */

using namespace std;

#include <stdio.h>
#include "users.h"
#include "channels.h"
#include "modules.h"
#include "helperfuncs.h"

/* $ModDesc: Provides channel mode +L (limit redirection) */


class ModuleRedirect : public Module
{
	Server *Srv;
	
 public:
 
	ModuleRedirect(Server* Me)
		: Module::Module(Me)
	{
		Srv = Me;
		Srv->AddExtendedMode('L',MT_CHANNEL,false,1,0);
	}
	
	virtual int OnExtendedMode(userrec* user, void* target, char modechar, int type, bool mode_on, string_list &params)
	{
		if ((modechar == 'L') && (type == MT_CHANNEL))
		{
			if (mode_on)
			{
				std::string ChanToJoin = params[0];
				chanrec *c;

				if (!IsValidChannelName(ChanToJoin.c_str()))
				{
					WriteServ(user->fd,"403 %s %s :Invalid channel name",user->nick, ChanToJoin.c_str());
					return 0;
				}

				c = Srv->FindChannel(ChanToJoin);
				if (c)
				{
					/* Fix by brain: Dont let a channel be linked to *itself* either */
					if ((c == target) || (c->IsModeSet('L')))
					{
						WriteServ(user->fd,"690 %s :Circular redirection, mode +L to %s not allowed.",user->nick,params[0].c_str());
     						return 0;
					}
				}
			}
			return 1;
		}
		return 0;
	}

	void Implements(char* List)
	{
		List[I_On005Numeric] = List[I_OnUserPreJoin] = List[I_OnExtendedMode] = 1;
	}

	virtual void On005Numeric(std::string &output)
	{
		InsertMode(output, "L", 3);
	}
	
	virtual int OnUserPreJoin(userrec* user, chanrec* chan, const char* cname)
	{
		if (chan)
		{
			if (chan->IsModeSet('L') && chan->limit)
			{
				if (Srv->CountUsers(chan) >= chan->limit)
				{
					std::string channel = chan->GetModeParameter('L');
					WriteServ(user->fd,"470 %s :%s has become full, so you are automatically being transferred to the linked channel %s",user->nick,cname,channel.c_str());
					Srv->JoinUserToChannel(user,channel.c_str(),"");
					return 1;
				}
			}
		}
		return 0;
	}

	virtual ~ModuleRedirect()
	{
	}
	
	virtual Version GetVersion()
	{
		return Version(1,0,0,0,VF_STATIC|VF_VENDOR);
	}
};


class ModuleRedirectFactory : public ModuleFactory
{
 public:
	ModuleRedirectFactory()
	{
	}
	
	~ModuleRedirectFactory()
	{
	}
	
	virtual Module * CreateModule(Server* Me)
	{
		return new ModuleRedirect(Me);
	}
	
};


extern "C" void * init_module( void )
{
	return new ModuleRedirectFactory;
}

