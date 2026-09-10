#pragma once

#include <string>
#include <unordered_map>

#include "tessinterfaces_global.h"

class TESSINTERFACES_EXPORT ITessStorage {
protected:
	std::unordered_map<std::string, std::string> mPaths;

public:
	enum IO: unsigned char {
		NullIO = 0,
		Read = 1,
		Write = 2,
		Binary = 4	// 默认都是文本，只有二进制需要设
	}; 

	enum StorageType : unsigned char {
		Local
	};

	ITessStorage() = default;
	ITessStorage(StorageType type): mStorageType(type){}

	StorageType mStorageType;

	virtual size_t write(const std::string& bytes, const std::string& path, unsigned char io = NullIO) = 0;

	virtual std::string read(const std::string& path, unsigned char io = NullIO) = 0;

	virtual void storeNewPath(const std::string& key, const std::string& path, unsigned char io = NullIO) {
		mPaths[key] = path;
	}

	virtual std::string getPathByKey(const std::string& key = "default") const {
		auto iter = mPaths.find(key);
		if (iter != mPaths.end())
			return iter->second;
		else
			return "";
	}

	virtual bool hasPathOfKey(const std::string& key) const {
		return mPaths.count(key) != 0;
	}

	virtual ~ITessStorage() {
		
	}
};

