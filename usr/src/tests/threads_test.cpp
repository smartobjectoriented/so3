/*
 * Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

#include <unistd.h>

struct ThreadArgs {
	long thread_id;
	std::mutex *mtx;
};

// Thread function
long simple_thread(ThreadArgs args)
{
	std::cout << "Thread " << args.thread_id << ": Executing" << std::endl;

	// Lock the mutex
	args.mtx->lock();
	std::cout << "Thread " << args.thread_id << ": Locked mutex" << std::endl;

	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << "Thread " << args.thread_id << ": Unlocking mutex" << std::endl;
	args.mtx->unlock();

	std::cout << "Thread " << args.thread_id << ": Thread finished" << std::endl;

	return args.thread_id;
}

int main()
{
	std::mutex mtx;

	printf("== C++ threads tests ==\n");

	// Create thread arguments
	ThreadArgs args1{ 1, &mtx };
	ThreadArgs args2{ 2, &mtx };

	// Store threads and their return values
	std::vector<std::thread> threads;
	long ret1, ret2;

	// Launch threads
	std::thread t1([&]() { ret1 = simple_thread(args1); });
	std::thread t2([&]() { ret2 = simple_thread(args2); });

	threads.push_back(std::move(t1));
	threads.push_back(std::move(t2));

	// Wait for threads to finish
	for (auto &t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	// Check return values
	if (ret1 != 1) {
		std::cout << "Unexpected return value for thread 1" << std::endl;
	}
	if (ret2 != 2) {
		std::cout << "Unexpected return value for thread 2" << std::endl;
	}

	std::cout << "All threads finished" << std::endl;
	return 0;
}
