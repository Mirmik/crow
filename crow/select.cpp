#include <crow/select.h>
#include <crow/tower.h>

#include <igris/math.h>
#include <sys/select.h>

#include <nos/print.h>

static std::vector<int> fds;
bool crow::add_unselect_to_fds = false;

void crow::select_collect_fds() 
{
	for (auto& gate : crow::gateway_list) 
	{
		int i = gate.get_fd();
		if (i >= 0) 
		{
			fds.push_back(i);
		}

		int strt = fds.size();
		fds.resize(fds.size() + gate.fds_total());
		gate.set_fds_to_array(&fds[strt]);
	}
	
	if (add_unselect_to_fds)
		fds.push_back(crow::unselect_pipe[0]);
}

void crow::add_select_fd(int fd) 
{
	fds.push_back(fd);
}

void crow::select() 
{
	fd_set read_fds;
	FD_ZERO(&read_fds);
	int nfds=0;
	
	for (int i : fds) 
	{
		FD_SET(i, &read_fds);
		nfds = __MAX__(i, nfds);		
	}

	int64_t timeout = crow::get_minimal_timeout();

	if (timeout < 0) 
	{
		::select(nfds+1, &read_fds, NULL, NULL, NULL);
	} 
	else 
	{
		struct timeval timeout_struct = {
				(time_t )(timeout / 1000),
				(suseconds_t)((timeout % 1000) * 1000)
		};
        ::select(nfds+1, &read_fds, NULL, NULL, &timeout_struct);
	}

}
