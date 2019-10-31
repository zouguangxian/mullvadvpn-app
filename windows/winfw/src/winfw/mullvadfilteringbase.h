#pragma once

#include "libwfp/providerbuilder.h"
#include "libwfp/sublayerbuilder.h"
#include "libwfp/conditionbuilder.h"
#include "libwfp/filterengine.h"
#include <memory>

class MullvadFilteringBase
{

public:

	MullvadFilteringBase() = delete;

	static void Init(wfp::FilterEngine &engine); // safe to call more than once
	//static void Purge(wfp::FilterEngine &engine); // TODO?

private:

	static std::unique_ptr<wfp::ProviderBuilder> Provider();
	static std::unique_ptr<wfp::SublayerBuilder> SublayerWhitelist();
	static std::unique_ptr<wfp::SublayerBuilder> SublayerBlacklist();
};
