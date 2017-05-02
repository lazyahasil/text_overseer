#pragma once

#include <memory>
#include <mutex>

template <typename T>
class Singleton
{
protected:
	Singleton() = default;

public:
	Singleton(const Singleton<T>& src) = delete;
	Singleton<T>& operator=(const Singleton<T>& rhs) = delete;

	static T& instance()
	{
		std::call_once(once_flag_, [&] {
			instance_ = std::make_unique<T>();
		});
		return *instance_.get();
	}

private:
	static std::unique_ptr<T> instance_;
	static std::once_flag once_flag_;
};

template <typename T>
std::unique_ptr<T> Singleton<T>::instance_;

template <typename T>
std::once_flag Singleton<T>::once_flag_;