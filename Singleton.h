#pragma once
#include <memory>
#include <mutex>
#include <iostream>
template<typename T>
class Singleton {
protected:
	Singleton() = default; 
	Singleton(const Singleton<T>&) = delete; 
	Singleton& operator=(const Singleton<T>& st) = delete;
	static std::shared_ptr<T> _instance; 
public:
	~Singleton() {
		std::cout << "Singleton destroyed." << std::endl; 
	}
	static std::shared_ptr<T> get_instance() {
		static std::once_flag init_flag; 
		std::call_once(init_flag, [&]() {
			_instance = std::shared_ptr<T>(new T());
			std::cout << "Singleton instance created." << std::endl;
			});
		return _instance; 
	}
	void print_address() {
		std::cout << "Singleton instance address: " << _instance.get() << std::endl; 
	}
};

template<typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr; 