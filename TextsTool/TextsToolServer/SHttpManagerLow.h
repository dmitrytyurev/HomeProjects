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
class SHttpManagerLow // PIMPL ���������� �� ������� � ����� SHttpManager ���������-��������� ��������� �������������
{
public:
	SHttpManagerLow(std::function<void(DeserializationBuffer&, SerializationBuffer&)> requestCallback); // ���������� �������, ������� ����� ������ ��� ������ �������� http-������� (���������� �����!). ������� ������ ������������ ����� �� ������
	void StartHttpListening();
	~SHttpManagerLow();
	void Update(double dt);

private:
	std::unique_ptr<SHttpManagerLowImpl> _pImpl;
};