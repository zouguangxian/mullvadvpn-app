#pragma once

#include "libwfp/providerbuilder.h"
#include "libwfp/sublayerbuilder.h"
#include "libwfp/conditionbuilder.h"
#include "libwfp/filterengine.h"
#include <memory>
#include <guiddef.h>

// Handles persistent WFP objects
class MullvadFilteringBase
{

public:

	static const GUID& ProviderGuid();
	static const GUID& SublayerWhitelistGuid();

	MullvadFilteringBase() = delete;

	static void Init(wfp::FilterEngine &engine); // safe to call more than once
	static void Purge(wfp::FilterEngine &engine);

	static std::unique_ptr<wfp::ProviderBuilder> Provider();
	static std::unique_ptr<wfp::SublayerBuilder> SublayerWhitelist();
	static std::unique_ptr<wfp::SublayerBuilder> SublayerBlacklist();

};
