#pragma once

#include "libwfp/filterengine.h"

class PersistentBlock
{
	PersistentBlock() = delete;

public:

	static const GUID &PersistentFilterBlockAll_Outbound_Ipv4();
	static const GUID &PersistentFilterBlockAll_Inbound_Ipv4();
	static const GUID &BootTimeFilterBlockAll_Outbound_Ipv4();
	static const GUID &BootTimeFilterBlockAll_Inbound_Ipv4();

	static bool Enable(wfp::FilterEngine &engine);
	static bool Disable(wfp::FilterEngine &engine);
};
