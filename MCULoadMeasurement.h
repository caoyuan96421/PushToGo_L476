#ifndef MCULOADMEASUREMENT_H_
#define MCULOADMEASUREMENT_H_

#include "mbed.h"

class MCULoadMeasurement
{
private:
	volatile bool active;
	Thread thd;
	uint64_t t_active;
	uint64_t t_total;
	float avg_usage;

	void _thread_entry();
	void reset();

	void setMCUActive(bool active);
	friend void idle_hook();
public:

	MCULoadMeasurement();
	virtual ~MCULoadMeasurement()
	{
	}

	float getCPUUsage();

	static MCULoadMeasurement &getInstance()
	{
		static MCULoadMeasurement m;
		return m;
	}
};

#endif // MCULOADMEASUREMENT_H_

