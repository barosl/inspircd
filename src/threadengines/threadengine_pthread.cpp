/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2010 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *	    the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "threadengines/threadengine_pthread.h"
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>

#ifdef HAS_EVENTFD
#include <sys/eventfd.h>

class ThreadSignalSocket : public EventHandler
{
 public:
	ThreadSignalSocket()
	{
		SetFd(eventfd(0, EFD_NONBLOCK));
		if (fd < 0)
			throw CoreException("Could not create eventfd " + std::string(strerror(errno)));
		ServerInstance->SE->AddFd(this, FD_WANT_FAST_READ | FD_WANT_NO_WRITE);
	}

	~ThreadSignalSocket()
	{
		close(fd);
	}

	void Notify()
	{
		eventfd_write(fd, 1);
	}

	void HandleEvent(EventType et, int errornum)
	{
		if (et == EVENT_READ)
		{
			eventfd_t dummy;
			eventfd_read(fd, &dummy);
			ServerInstance->Threads->result_loop();
		}
		else
		{
			ServerInstance->Threads->job_lock.lock();
			ServerInstance->Threads->result_s = NULL;
			ServerInstance->Threads->job_lock.unlock();
			ServerInstance->GlobalCulls.AddItem(this);
		}
	}
};

#else

class ThreadSignalSocket : public EventHandler
{
	int send_fd;
 public:
	ThreadSignalSocket()
	{
		int fds[2];
		if (pipe(fds))
			throw new CoreException("Could not create pipe " + std::string(strerror(errno)));
		SetFd(fds[0]);
		send_fd = fds[1];

		ServerInstance->SE->NonBlocking(fd);
		ServerInstance->SE->AddFd(this, FD_WANT_FAST_READ | FD_WANT_NO_WRITE);
	}

	~ThreadSignalSocket()
	{
		close(send_fd);
		close(fd);
	}

	void Notify()
	{
		static const char dummy = '*';
		write(send_fd, &dummy, 1);
	}

	void HandleEvent(EventType et, int errornum)
	{
		if (et == EVENT_READ)
		{
			char dummy[128];
			read(fd, dummy, 128);
			ServerInstance->Threads->result_loop();
		}
		else
		{
			ServerInstance->Threads->job_lock.lock();
			ServerInstance->Threads->result_s = NULL;
			ServerInstance->Threads->job_lock.unlock();
			ServerInstance->GlobalCulls.AddItem(this);
		}
	}
};
#endif

ThreadEngine::ThreadEngine() : result_s(NULL)
{
}

ThreadEngine::~ThreadEngine()
{
	delete result_s;
}

void* ThreadEngine::Runner::entry_point(void* parameter)
{
	/* Recommended by nenolod, signal safety on a per-thread basis */
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	Runner* te = static_cast<Runner*>(parameter);
	te->main_loop();
	return parameter;
}

ThreadEngine::Runner::Runner(ThreadEngine* t)
	: te(t), current(NULL)
{
	if (pthread_create(&id, NULL, entry_point, this) != 0)
		throw CoreException("Unable to create new thread: " + std::string(strerror(errno)));
}

ThreadEngine::Runner::~Runner()
{
	pthread_join(id, NULL);
}

void ThreadEngine::Submit(Job* job)
{
	Mutex::Lock lock(job_lock);
	if (threads.empty())
	{
		Runner* thread = new Runner(this);
		threads.push_back(thread);
	}
	if (result_s == NULL)
		result_s = new ThreadSignalSocket();
	submit_q.push_back(job);
	submit_s.signal_one();
}

void ThreadEngine::Runner::main_loop()
{
	te->job_lock.lock();
	while (1)
	{
		while (te->submit_q.empty())
			te->submit_s.wait(te->job_lock);

		current = te->submit_q.front();
		te->submit_q.pop_front();
		te->job_lock.unlock();
		
		current->run();

		te->job_lock.lock();
		te->result_q.push_back(current);
		current = NULL;

		if (te->result_s)
			te->result_s->Notify();
	}
	te->job_lock.unlock();
}

void ThreadEngine::result_loop()
{
	job_lock.lock();
	while (!result_q.empty())
	{
		Job* job = result_q.front();
		result_q.pop_front();
		job_lock.unlock();
		job->finish();
		job_lock.lock();
	}
	job_lock.unlock();
}
