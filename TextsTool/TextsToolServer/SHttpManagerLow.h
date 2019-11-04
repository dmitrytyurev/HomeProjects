#pragma once
#include <vector>
#include <stdint.h>
#include <functional>
#include "DeserializationBuffer.h"
#include "SerializationBuffer.h"

class SHttpManagerLowImpl;

//===============================================================================
//
//===============================================================================
class SHttpManagerLow // PIMPL реализация не пускает в класс SHttpManager платформо-зависимые системные заголовочники
{
public:
	SHttpManagerLow(std::function<void(DeserializationBuffer&, SerializationBuffer&)> requestCallback); // Запоминает функцию, которую будет дёргать при каждом входящем http-запросе (неосновной поток!). Функция должна сформировать ответ на запрос
	~SHttpManagerLow();
	void Update(double dt);


private:
	void StartHttpListening();

private:
	std::unique_ptr<SHttpManagerLowImpl> _pImpl;
};