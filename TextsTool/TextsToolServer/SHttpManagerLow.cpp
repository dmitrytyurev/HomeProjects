#include "pch.h"

#include <iostream>
#include <sstream>
#include <string>
#include <memory>

#include "SHttpManagerLow.h"
#include "SHttpManagerLowImpl.h"

void ExitMsg(const std::string& message);

//===============================================================================
//
//===============================================================================

SHttpManagerLow::SHttpManagerLow(std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&)> requestCallback)
{
	_pImpl = std::make_unique<SHttpManagerLowImpl>(requestCallback);
}

//===============================================================================
//
//===============================================================================

void SHttpManagerLow::StartHttpListening()
{
	_pImpl->StartHttpListening();
}

//===============================================================================
//
//===============================================================================

SHttpManagerLow::~SHttpManagerLow()
{
}
