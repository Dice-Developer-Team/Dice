#pragma once
#ifndef __STORAGEBASE__
#define __STORAGEBASE__
#include <string>
class StorageBase
{
protected:
	const std::string FilePath;
public:
	virtual void read() = 0;
	virtual void save() const = 0;
	StorageBase(const std::string& FilePath);
	virtual ~StorageBase();
};
#endif /*__STORAGEBASE__*/

