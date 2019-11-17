#include "pch.h"

#include <iostream>
#include <sstream>
#include <string>
#include <memory>

#include "SHttpManagerLow.h"
#include "SHttpManagerLowImpl.h"

void ExitMsg(const std::string& message);

//===============================================================================

SHttpManagerLow::SHttpManagerLow(std::function<void(DeserializationBuffer&, SerializationBuffer&)> requestCallback)
{
	_pImpl = std::make_unique<SHttpManagerLowImpl>(requestCallback);
	StartHttpListening();
}

//===============================================================================

void SHttpManagerLow::StartHttpListening()
{
	_pImpl->StartHttpListening();
}

//===============================================================================

SHttpManagerLow::~SHttpManagerLow()
{
}

//===============================================================================

void SHttpManagerLow::Update(double dt)
{
	_pImpl->Update(dt);
}