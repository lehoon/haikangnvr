#pragma once

#include <string>
#include <map>

#include <stdint.h>

#define RESULT_OK      0
#define RESULT_ERROR   1

class CConfigure
{
public:
	CConfigure(void);
	~CConfigure(void);

public:
	int Init(const std::string& file);
	void Dump2LogFile();

	std::string GetValueAsString(const std::string& group,
		const std::string& key,
		const std::string& default_value) const;
	uint32_t GetValueAsUint32(const std::string& group,
		const std::string& key,
		uint32_t default_value) const;
	double GetValueAsDouble(const std::string& group,
		const std::string& key,
		double default_value) const;
	bool GetValueAsBool(const std::string& group,
		const std::string& key,
		bool  default_value) const;

private:
	std::map<std::string, std::map<std::string, std::string> > items_;
};

