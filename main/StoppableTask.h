#pragma once

#ifndef _STOPPABLETASK_BB60A0DF_BADC_4384_8978_B7403659030F
#define _STOPPABLETASK_BB60A0DF_BADC_4384_8978_B7403659030F

#include <chrono>

/*
 * StoppableTask provides a mechanism for worker threads to efficiently wait
 * for stop requests alongside other events.
 *
 * Usage mode 1: Timed events only
 *   If your thread only needs to wake at scheduled times, use IsStopRequested()
 *   with the milliseconds until the next wakeup:
 *
 *     while (!IsStopRequested(5000)) {
 *         // Do periodic work every 5 seconds
 *     }
 *
 *   Note: The problem with this approach is that when real work arrives, it
 *   doesn't get processed until the thread has finished waiting to see if it's
 *   time to die. Use mode 2 for responsive event handling.
 *
 * Usage mode 2: Event-driven with select()
 *   If your thread needs to wake promptly on I/O events, use GetStopFd() in
 *   a select() call alongside your work file descriptors:
 *
 *     int stop_fd = GetStopFd();
 *     while (true) {
 *         fd_set rfds;
 *         FD_ZERO(&rfds);
 *         FD_SET(stop_fd, &rfds);
 *         FD_SET(socket_fd, &rfds);
 *         int maxfd = (stop_fd > socket_fd) ? stop_fd : socket_fd;
 *
 *         struct timeval tv = {5, 0};  // 5 second timeout
 *         if (select(maxfd + 1, &rfds, nullptr, nullptr, &tv) > 0) {
 *             if (FD_ISSET(stop_fd, &rfds))
 *                 break;  // Stop requested
 *             if (FD_ISSET(socket_fd, &rfds))
 *                 // Handle incoming data
 *         }
 *         // Handle timeout for periodic work
 *     }
 */

class StoppableTask
{
public:
	StoppableTask();
	StoppableTask(StoppableTask &&obj) noexcept;
	StoppableTask &operator=(StoppableTask &&obj) noexcept;
	~StoppableTask();
	bool IsStopRequested(const int timeMS = 0);
	void RequestStop();
	void RequestStart();
	int GetStopFd();

	// Select helpers
	void ClearSelectFds();
	void SetSelectFd(int fd, bool wantRead, bool wantWrite, bool wantExcept);
	void SelectTimeout(int seconds, int microseconds = 0);
	template<typename Rep, typename Period>
	void SelectTimeout(const std::chrono::duration<Rep, Period>& duration)
	{
		auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration);
		SelectTimeout(us.count() / 1000000, us.count() % 1000000);
	}
	int DoSelect();
	bool FdIsReadable(int fd) const { return FD_ISSET(fd, &m_rfds); }
	bool FdIsWritable(int fd) const { return FD_ISSET(fd, &m_wfds); }
	bool FdIsExcept(int fd) const { return FD_ISSET(fd, &m_efds); }
	bool FdStopIsSet() const { return m_stopfd[0] != INVALID_SOCKET && FD_ISSET(m_stopfd[0], &m_rfds); }

private:
	SOCKET m_stopfd[2];
	fd_set m_rfds, m_wfds, m_efds;
	int m_nfds;
	struct timeval m_timeout;
};

#endif //_STOPPABLETASK_BB60A0DF_BADC_4384_8978_B7403659030F
