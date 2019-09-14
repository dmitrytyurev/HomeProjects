#pragma once
#include <vector>
#include <stdint.h>
#include <functional>

class SHttpManagerLowImpl;

//===============================================================================
//
//===============================================================================
class SHttpManagerLow // PIMPL реализация не пускает в класс SHttpManager платформо-зависимые системные заголовочники
{
public:
	SHttpManagerLow(std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> requestCallback); // Запоминает функцию, которую будет дёргать при каждом входящем http-запросе (неосновной поток!). Функция должна сформировать ответ на запрос
	void StartHttpListening();
	~SHttpManagerLow();
	void Update(double dt);

private:
	std::unique_ptr<SHttpManagerLowImpl> _pImpl;
};